#include "kalaOKServer.h"

#include <QTcpServer>
#include <QMessageBox>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QJsonObject>
#include <QNetworkProxyFactory>
#include <QDebug>
#include <QSqlQuery>

kalaOKServer::kalaOKServer(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
	setWindowTitle(u8"����з����OK���ϵͳ�����");
	init();
}

void kalaOKServer::init()
{
	server = new QTcpServer();
	QNetworkProxyFactory::setUseSystemConfiguration(false);

	if (!server->listen(QHostAddress::Any, 9999))
	{
		qDebug() << "�������˼���ʧ�ܣ�++" + server->errorString();
	}
	else
	{
		connect(server, SIGNAL(newConnection()), this, SLOT(ServerNewConnection()));
		QMessageBox::information(this, u8"QT����ͨ��", u8"�������˼����ɹ���");
		initDatabase();
	}
}

void kalaOKServer::initDatabase()
{
	db = QSqlDatabase::addDatabase("QMYSQL");//����MySQL���ݿ�
	db.setHostName("127.0.0.1");//����ip
	db.setPort(3306);
	db.setDatabaseName("ruanjian");//���ݿ�����
	db.setUserName("root");
	db.setPassword("123456");
	//�����ݿ�
	if (db.open() == false)
	{
		ui.serverInfo->append(db.lastError().text() + '\n');//����ʧ��
	}
	else
	{
		ui.serverInfo->append(u8"�������ݿ�ɹ�\n");//���ӳɹ�
	}
}

void kalaOKServer::sendMessage(QJsonObject json)
{
	QJsonDocument document;
	document.setObject(json);
	socket->write(document.toJson(QJsonDocument::Compact));
}

void kalaOKServer::responseOrder()
{
	QString musicName = receiveJson.value("musicName").toString().split('-').first();
	QSqlQuery query;
	if (query.exec(u8"update song set browseNums = browseNums+1 where song = \"" + musicName + "\";"))
	{
		ui.serverInfo->append(u8"���ɹ�\n");
		QJsonObject json;
		json.insert("responseType", "order");
		json.insert("result", 1);
		json.insert("music", musicName);
		sendMessage(json);
	}
	else
	{
		ui.serverInfo->append(query.lastError().text() + u8"�����ʧ��");
	}
	
}

void kalaOKServer::responseHotMusic()
{
	QSqlQuery query;
	if (query.exec(u8"SELECT song,singer,browseNums from song ORDER BY  browseNums  DESC LIMIT 0, 10;"))
	{
		QJsonArray hotMusics,singers;
		QJsonObject json;
		while (query.next())
		{
			hotMusics.append(query.value(0).toString());
			singers.append(query.value(1).toString());
		}
		json.insert("responseType", "hotMusic");
		json.insert("musicNames", singers);
		json.insert("singers", hotMusics);
		sendMessage(json);
		ui.serverInfo->append(u8"�ȸ���ͳɹ�\n");
	}
	else
	{
		ui.serverInfo->append(query.lastError().text() + u8"�ȸ����ʧ��\n");
	}
}

void kalaOKServer::responSesearch()
{
	QSqlQuery query = QSqlQuery(db);
	QString str = "select singer,song from song where song = \"" + receiveJson.value("musicName").toString() + "\""
					" or singer = \"" + receiveJson.value("musicName").toString() + "\";";
	QJsonArray Singer, Song;
	if (query.exec(str))
	{
		ui.serverInfo->append(u8"�ظ���ѯ�ɹ�");
		while (query.next())
		{
			Singer.append(query.value(0).toString());
			ui.serverInfo->append(query.value(0).toString());
			Song.append(query.value(1).toString());
			ui.serverInfo->append(query.value(1).toString());
		}
		QJsonObject json;
		json.insert("responseType", "search");
		json.insert("musicNames", Song);
		json.insert("singers", Singer);
		sendMessage(json);//������Ӧ
	}
	else
	{
		ui.serverInfo->append(query.lastError().text() + u8"��ѯʧ��");
	}
}

void kalaOKServer::ServerNewConnection()
{
	socket = server->nextPendingConnection();
	ui.serverInfo->append(u8"�½����ӣ�" + socket->peerAddress().toString());
	connect(socket, SIGNAL(readyRead()), this, SLOT(ServerReadData()));
}

void kalaOKServer::ServerReadData()
{
	QByteArray ba = socket->readAll();
	QJsonDocument document = QJsonDocument::fromJson(ba);
	receiveJson = document.object();
	receiveJson = document.object();
	ui.serverInfo->append(u8"�յ�����" + receiveJson.value("questType").toString());
	if (receiveJson.value("questType").toString() == "order")responseOrder();
	if (receiveJson.value("questType").toString() == "hotMusic")responseHotMusic();
	if (receiveJson.value("questType").toString() == "search")responSesearch();
}

void kalaOKServer::on_btn_exec_clicked()
{
	QSqlQuery query;
	if (query.exec(ui.sqlString->text()))
		ui.serverInfo->append(u8"ִ�гɹ�");
	else
		ui.serverInfo->append(u8"ִ��ʧ��");
}
