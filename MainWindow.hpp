#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>
#include <QDir>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "ui_MainWindow.h"

class MainWindow: public QMainWindow {
	Q_OBJECT;
public:
	MainWindow();

public slots:
	void checkConnection();
	void reconnect();
	void handleReplyData();
	void handleReplyFinished();
	void handleReplyError(QNetworkReply::NetworkError);
	void browse();

	void setUrl(const QString &);
	void log(const QString &);

	void on_logCheckBox_toggled(bool);

protected:
	void handleBuf(const QByteArray &src, const QByteArray &buf);
	void reply(const QByteArray &tgt, quint16 id, const QJsonValue &msg = QJsonValue());

	QDir getPath() const;

	void handle_fileExists(const QByteArray &src, const QString &file);
	void handle_getDir(const QByteArray &src, const QString &dir);
	void handle_fileGetContents(const QByteArray &src, const QString &file);
	void handle_fileGetSize(const QByteArray &src, const QString &file);

private:
	Ui::MainWindow ui;
	QNetworkAccessManager net;
	QNetworkAccessManager net_out;
	QNetworkReply *net_reply;
	QTimer timer;
	bool new_connection;
	bool log_enabled;
	QByteArray buf;
	QByteArray our_id;
};

