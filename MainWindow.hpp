#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>
#include "ui_MainWindow.h"

class MainWindow: public QMainWindow {
	Q_OBJECT;
public:
	MainWindow();

public slots:
	void checkConnection();
	void handleRcData();

	void setUrl(const QString &);
	void log(const QString &);

protected:
	void handleBuf(const QByteArray &src, const QByteArray &buf);
	void reply(const QByteArray &tgt, quint32 packet_id, quint16 id, const QByteArray &msg = QByteArray());

private:
	Ui::MainWindow ui;
	QTcpSocket *rc;
	QTimer timer;
	bool new_connection;
	QByteArray buf;
	QByteArray our_id;
};

