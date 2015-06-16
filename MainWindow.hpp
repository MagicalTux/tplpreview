#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>
#include <QDir>
#include "ui_MainWindow.h"

class MainWindow: public QMainWindow {
	Q_OBJECT;
public:
	MainWindow();

public slots:
	void checkConnection();
	void reconnect();
	void handleRcData();
	void browse();

	void setUrl(const QString &);
	void log(const QString &);

	void on_logCheckBox_toggled(bool);

protected:
	void handleBuf(const QByteArray &src, const QByteArray &buf);
	void reply(const QByteArray &tgt, quint32 packet_id, quint16 id, const QByteArray &msg = QByteArray());

	QDir getPath() const;

	void handle_fileExists(const QByteArray &src, quint32 packet_id, const QString &file);
	void handle_getDir(const QByteArray &src, quint32 packet_id, const QString &dir);
	void handle_fileGetContents(const QByteArray &src, quint32 packet_id, const QString &file);
	void handle_fileGetSize(const QByteArray &src, quint32 packet_id, const QString &file);

private:
	Ui::MainWindow ui;
	QTcpSocket *rc;
	QTimer timer;
	bool new_connection;
	bool log_enabled;
	QByteArray buf;
	QByteArray our_id;
};

