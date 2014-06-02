#include "MainWindow.hpp"

void MainWindow::handle_fileExists(const QByteArray &src, quint32 packet_id, const QString &file) {
	qDebug("file_exists(%s)", qPrintable(file));
	QDir path = getPath();
	if (!path.exists()) {
		reply(src, packet_id, 0xdead, "Root is not valid");
		return;
	}
	if (file.contains("../")) {
		reply(src, packet_id, 0xdead, "File name is not valid");
		return;
	}
	QFile sub(path.absoluteFilePath(file));
	if (sub.exists()) {
		reply(src, packet_id, 0xff, QByteArray(1, '\x01'));
	} else {
		reply(src, packet_id, 0xff, QByteArray(1, '\x00'));
	}
}

static void handle_getDir_recurse(const QDir &base, QDir dir, QDataStream &s) {
	QFileInfoList files = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
	foreach(const QFileInfo &fi, files) {
		if (fi.isDir()) {
			handle_getDir_recurse(base, fi.dir(), s);
			continue;
		}
		if (fi.isFile()) {
			// make relative path
			QByteArray path = base.relativeFilePath(fi.absoluteFilePath()).toUtf8();
			qDebug("file: %s", qPrintable(path));
			s << (quint16)path.length();
			s.writeRawData(path.data(), path.length());
			QFile f(fi.absoluteFilePath());
			f.open(QIODevice::ReadOnly);
			QByteArray buf = f.readAll();
			f.close();
			s << (quint32)buf.length();
			s.writeRawData(buf.data(), buf.length());
		}
	}
}

void MainWindow::handle_getDir(const QByteArray &src, quint32 packet_id, const QString &dir) {
	qDebug("getDir(%s)", qPrintable(dir));
	QDir path = getPath();
	if (!path.exists()) {
		reply(src, packet_id, 0xdead, "Root is not valid");
		return;
	}
	if (dir.contains("../")) {
		reply(src, packet_id, 0xdead, "File name is not valid");
		return;
	}
	QDir sub(path.absoluteFilePath(dir));
	if (!sub.exists()) {
		reply(src, packet_id, 0xdead, "Directory not found");
		return;
	}

	QByteArray res;
	QDataStream s(&res, QIODevice::WriteOnly);
	handle_getDir_recurse(sub, sub, s);

	reply(src, packet_id, 0xff, res);
}

void MainWindow::handle_fileGetContents(const QByteArray &src, quint32 packet_id, const QString &file) {
	qDebug("file_get_contents(%s)", qPrintable(file));
	QDir path = getPath();
	if (!path.exists()) {
		reply(src, packet_id, 0xdead, "Root is not valid");
		return;
	}
	if (file.contains("../")) {
		reply(src, packet_id, 0xdead, "File name is not valid");
		return;
	}
	QFile sub(path.absoluteFilePath(file));
	if (!sub.exists()) {
		reply(src, packet_id, 0xdead, "File not found");
		return;
	}
	if (!sub.open(QIODevice::ReadOnly)) {
		reply(src, packet_id, 0xdead, "Could not open file for reading");
		return;
	}
	reply(src, packet_id, 0xff, sub.readAll());
	sub.close();
}

