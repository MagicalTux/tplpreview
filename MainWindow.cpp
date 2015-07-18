#include <QFileDialog>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QJsonDocument>
#include <QJsonObject>
#include "MainWindow.hpp"

#define TPLPREVIEW_DOMAIN "tplpreview.com"

MainWindow::MainWindow() {
	ui.setupUi(this);
	connect(ui.btn_browse, SIGNAL(clicked()), this, SLOT(browse()));
	connect(ui.btn_regen, SIGNAL(clicked()), this, SLOT(reconnect()));
	net_reply = NULL;
	log_enabled = true;

	connect(&timer, SIGNAL(timeout()), this, SLOT(checkConnection()));
	timer.setSingleShot(false);
	timer.start(30000);

	setUrl("(connecting...)");
	log("Connecting to system...");
	checkConnection();
}

void MainWindow::reconnect() {
	log("Triggering reconnection...");
	if (net_reply) {
		net_reply->close();
		net_reply->deleteLater();
		net_reply = NULL;
	}

	net.cookieJar()->deleteLater();
	net.setCookieJar(new QNetworkCookieJar());

	checkConnection();
}

void MainWindow::checkConnection() {
	if (!net_reply) {
//		qDebug("Do query now");
		QNetworkRequest req(QUrl("http://" TPLPREVIEW_DOMAIN "/_special/stream"));
		req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
		req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
		req.setRawHeader("Accept-Encoding", "none");
		net_reply = net.get(req);
		connect(net_reply, SIGNAL(readyRead()), this, SLOT(handleReplyData()));
		connect(net_reply, SIGNAL(finished()), this, SLOT(handleReplyFinished()));
		connect(net_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleReplyError(QNetworkReply::NetworkError)));
		new_connection = true;
	}
}

void MainWindow::handleReplyError(QNetworkReply::NetworkError e) {
	qDebug("ERROR");
	if (net_reply) {
		net_reply->close();
		net_reply->deleteLater();
		net_reply = NULL;
	}

	checkConnection();
}

void MainWindow::handleReplyData() {
	if (!net_reply) return;
	while(net_reply->canReadLine()) {
		QByteArray lin = net_reply->readLine();

		if (new_connection) {
			// this is our ID
			our_id = lin.trimmed();
//			log("Connection ready, ID: "+our_id);
			setUrl(QString("http://")+our_id+"." TPLPREVIEW_DOMAIN "/");
			new_connection = false;
			continue;
		}
		// parse json
		QJsonParseError e;
		QJsonDocument doc = QJsonDocument::fromJson(lin.trimmed(), &e);
		if (!doc.isObject()) continue; // wtf?

		int code = doc.object().value("code").toInt();
		QByteArray reply_to = doc.object().value("reply_to").toString().toUtf8();
		QString data = doc.object().value("data").toString();

		switch(code) {
			case 0x00: // LOG
				log(data);
				reply(reply_to, 0xff);
				break;
			case 0x01: // FILE_EXISTS
				handle_fileExists(reply_to, data);
				break;
			case 0x02: // GET_DIR (return contents of all files in dir recursively)
				handle_getDir(reply_to, data);
				break;
			case 0x03: // FILE_GET_CONTENTS
				handle_fileGetContents(reply_to, data);
				break;
			case 0x04: // FILE_GET_SIZE
				handle_fileGetSize(reply_to, data);
				break;
		}
	}
}

void MainWindow::handleReplyFinished() {
	if ((net_reply) && (net_reply->isFinished())) {
		net_reply->deleteLater();
		net_reply = NULL;
	}
	checkConnection();
}

void MainWindow::browse() {
	QString res = QFileDialog::getExistingDirectory(this, tr("Select directory containing git root"), ui.base_path->text(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (res.isEmpty()) return;
	ui.base_path->setText(res);
}

QDir MainWindow::getPath() const {
	return QDir(ui.base_path->text());
}

void MainWindow::reply(const QByteArray &tgt, quint16 id, const QJsonValue &payload) {
	QJsonObject obj;
	obj.insert("code", id);
	obj.insert("payload", payload);

	QNetworkRequest req(QUrl("http://" TPLPREVIEW_DOMAIN "/_special/stream/" + tgt));
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	net_out.post(req, QJsonDocument(obj).toJson());
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

