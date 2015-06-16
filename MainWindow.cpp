#include <QFileDialog>
#include "MainWindow.hpp"

MainWindow::MainWindow() {
	ui.setupUi(this);
	connect(ui.btn_browse, SIGNAL(clicked()), this, SLOT(browse()));
	connect(ui.btn_regen, SIGNAL(clicked()), this, SLOT(reconnect()));
	rc = NULL;
	log_enabled = true;

	connect(&timer, SIGNAL(timeout()), this, SLOT(checkConnection()));
	timer.setSingleShot(false);
	timer.start(30000);

	checkConnection();
}

void MainWindow::reconnect() {
	log("Triggering reconnection...");
	if (rc) {
		if (rc->state() == QAbstractSocket::ConnectedState)
			rc->disconnect();
		delete rc;
		rc = NULL;
	}
	checkConnection();
}

void MainWindow::checkConnection() {
	// check connect
	if (rc) {
		if (rc->state() == QAbstractSocket::ConnectedState) {
			// send ping
			if (!new_connection)
				rc->write(our_id + QByteArray(2, '\0'));
			return;
		}
		delete rc;
		rc = NULL;
	}

	rc = new QTcpSocket(this);
	rc->connectToHost("tplpreview.com", 55555);
	new_connection = true; // first 16 bytes are our ID

	connect(rc, SIGNAL(readyRead()), this, SLOT(handleRcData()));
	setUrl("(connecting...)");
	log("Connecting to system...");
}

void MainWindow::handleBuf(const QByteArray &src, const QByteArray &buf) {
	// get ID from buf
	QDataStream r(buf);
	
	quint32 packet_id;
	quint16 id;

	r >> packet_id >> id;

//	qDebug("Parser: packet_id = %d id = %d", packet_id, id);

	switch(id) {
		case 0x00: // LOG
			log(QString::fromUtf8(buf.mid(6)));
			reply(src, packet_id, 0xff);
			break;
		case 0x01: // FILE_EXISTS
			handle_fileExists(src, packet_id, QString::fromUtf8(buf.mid(6)));
			break;
		case 0x02: // GET_DIR (return contents of all files in dir recursively)
			handle_getDir(src, packet_id, QString::fromUtf8(buf.mid(6)));
			break;
		case 0x03: // FILE_GET_CONTENTS
			handle_fileGetContents(src, packet_id, QString::fromUtf8(buf.mid(6)));
			break;
		case 0x04: // FILE_GET_SIZE
			handle_fileGetSize(src, packet_id, QString::fromUtf8(buf.mid(6)));
			break;
	}
}

void MainWindow::browse() {
	QString res = QFileDialog::getExistingDirectory(this, tr("Select directory containing git root"), ui.base_path->text(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (res.isEmpty()) return;
	ui.base_path->setText(res);
}

QDir MainWindow::getPath() const {
	return QDir(ui.base_path->text());
}

void MainWindow::reply(const QByteArray &tgt, quint32 packet_id, quint16 id, const QByteArray &payload) {
	if (rc == NULL) return;
	qDebug(" -> Reply %d [%x] with %d bytes of data", packet_id, id, payload.length());
	if (id == 0xdead) {
		qDebug(" -> Exception: %s", qPrintable(payload));
	}
	QByteArray header;
	QDataStream header_w(&header, QIODevice::WriteOnly);
	header_w.writeRawData(tgt.data(), tgt.length());

	QByteArray msg;
	{
		QDataStream w(&msg, QIODevice::WriteOnly);
		w << packet_id << id;
	}
	msg.append(payload);

	if (msg.length() < 0x8000) {
		quint16 l = msg.length();
		header_w << l;
		rc->write(header + msg);
		return;
	}
	quint16 l;
	quint32 payload_len = msg.length();
	l = ((payload_len >> 16) & 0x7fff) | 0x8000;
	header_w << l;
	l = payload_len & 0xffff;
	header_w << l;
	rc->write(header);
	rc->write(msg);
}

void MainWindow::handleRcData() {
	buf.append(rc->readAll());

	if (new_connection) {
		if (buf.length() < 16) return;
		our_id = buf.mid(0, 16);
		QByteArray id = our_id.toHex();
		log("Connection ready, ID: "+id);
		setUrl(QString("http://")+id+".tplpreview.com/");
		buf.remove(0, 16);
		new_connection = false;
	}

	// check data
	while(true) {
		if (buf.length() < 18) return;
		quint32 len;
		quint16 len_min;
		quint32 header_len = 18;

		QByteArray tgt = buf.mid(0, 16);
		QDataStream r(buf);
		r.skipRawData(16);

		r >> len_min;
		len = len_min;
		if (len_min & 0x8000) {
			if (buf.length() < 20) return;
			r >> len_min;
			len = ((len & 0x7fff) << 16) | len_min;
			header_len = 20;
		}
		if ((quint64)buf.length() < len+header_len)
			return; // not enough yet

		if ((len == 0) && (tgt == our_id)) {
			qDebug("Ping? Pong!");
			buf.remove(0, len+header_len);
			continue;
		}

//		qDebug("Got %d bytes of data from %s", len, qPrintable(tgt.toHex()));
		handleBuf(tgt, buf.mid(header_len, len));

		buf.remove(0, len+header_len);
	}
}

void MainWindow::setUrl(const QString &url) {
	ui.label_url->setText(url);
}

void MainWindow::log(const QString &msg) {
	if (!log_enabled) return;
	qDebug("log(%s)", qPrintable(msg));
	ui.list_log->addItem(msg);
}

void MainWindow::on_logCheckBox_toggled(bool checked) {
	log_enabled = checked;
	if (checked) {
		// enable logs
		ui.logFrame->setEnabled(true);
	} else {
		ui.logFrame->setEnabled(false);
		ui.list_log->clear();
	}
}

