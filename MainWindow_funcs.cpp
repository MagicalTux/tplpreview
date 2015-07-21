#include "MainWindow.hpp"
#include <QJsonObject>

void MainWindow::handle_fileExists(const QByteArray &src, const QString &file) {
	qDebug("file_exists(%s)", qPrintable(file));
	QDir path = getPath();
	if (!path.exists()) {
		reply(src, 0xdead, QStringLiteral("Root is not valid"));
		return;
	}
	if (file.contains("../")) {
		reply(src, 0xdead, QStringLiteral("File name is not valid"));
		return;
	}
	QFile sub(path.absoluteFilePath(file));
	if (sub.exists()) {
		reply(src, 0xff, true);
	} else {
		reply(src, 0xff, false);
	}
}

static void handle_getDir_recurse(const QDir &base, QDir dir, QJsonObject &result) {
	QFileInfoList files = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
	foreach(const QFileInfo &fi, files) {
		if (fi.isDir()) {
			handle_getDir_recurse(base, fi.absoluteFilePath(), result);
			continue;
		}
		if (fi.isFile()) {
			// make relative path
			QByteArray path = base.relativeFilePath(fi.absoluteFilePath()).toUtf8();
			qDebug("file: %s", qPrintable(path));

			QFile f(fi.absoluteFilePath());
			f.open(QIODevice::ReadOnly);
			QByteArray buf = f.readAll();
			f.close();

			result.insert(base.relativeFilePath(fi.absoluteFilePath()), QString(buf.toBase64()));
		}
	}
}

void MainWindow::handle_getDir(const QByteArray &src, const QString &dir) {
	qDebug("getDir(%s)", qPrintable(dir));
	QDir path = getPath();
	if (!path.exists()) {
		reply(src, 0xdead, QStringLiteral("Root is not valid"));
		return;
	}
	if (dir.contains("../")) {
		reply(src, 0xdead, QStringLiteral("File name is not valid"));
		return;
	}
	QDir sub(path.absoluteFilePath(dir));
	if (!sub.exists()) {
		reply(src, 0xdead, QStringLiteral("Directory not found"));
		return;
	}

	QJsonObject res;
	handle_getDir_recurse(sub, sub, res);

	reply(src, 0xff, res);
}

void MainWindow::handle_fileGetContents(const QByteArray &src, const QString &file) {
	qDebug("file_get_contents(%s)", qPrintable(file));
	QDir path = getPath();
	if (!path.exists()) {
		reply(src, 0xdead, QStringLiteral("Root is not valid"));
		return;
	}
	if (file.contains("../")) {
		reply(src, 0xdead, QStringLiteral("File name is not valid"));
		return;
	}
	QFile sub(path.absoluteFilePath(file));
	if (!sub.exists()) {
		reply(src, 0xdead, QStringLiteral("File not found"));
		return;
	}
	if (!sub.open(QIODevice::ReadOnly)) {
		reply(src, 0xdead, QStringLiteral("Could not open file for reading"));
		return;
	}
	reply(src, 0xff, QString(sub.readAll().toBase64()));
	sub.close();
}

void MainWindow::handle_fileGetSize(const QByteArray &src, const QString &file) {
	qDebug("file_get_size(%s)", qPrintable(file));
	QDir path = getPath();
	if (!path.exists()) {
		reply(src, 0xdead, QStringLiteral("Root is not valid"));
		return;
	}
	if (file.contains("../")) {
		reply(src, 0xdead, QStringLiteral("File name is not valid"));
		return;
	}
	QFile sub(path.absoluteFilePath(file));
	if (!sub.exists()) {
		reply(src, 0xdead, QStringLiteral("File not found"));
		return;
	}
	reply(src, 0xff, sub.size());
}

