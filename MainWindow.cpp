#include "MainWindow.hpp"

MainWindow::MainWindow() {
	ui.setupUi(this);
	rc = NULL;

	checkConnection();

	connect(&timer, SIGNAL(timeout()), this, SLOT(checkConnection()));
	timer.setSingleShot(false);
	timer.start(3000);
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
	rc->connectToHost("118.27.0.3", 55555);
	new_connection = true; // first 16 bytes are our ID

	connect(rc, SIGNAL(readyRead()), this, SLOT(handleRcData()));
	setUrl("(connecting...)");
	log("Connecting to system...");
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

		qDebug("Got %d bytes of data from %s", len, qPrintable(tgt.toHex()));
		buf.remove(0, len+header_len);
	}
}

void MainWindow::setUrl(const QString &url) {
	ui.label_url->setText(url);
}

void MainWindow::log(const QString &msg) {
	ui.list_log->addItem(msg);
}
