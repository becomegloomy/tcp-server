#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTcpServer;
class QTcpSocket;
class QTextEdit;

class ServerWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ServerWindow(QWidget *parent = nullptr);

private slots:
    void toggleServer();
    void onNewConnection();
    void sendMessage();

private:
    void readClient(QTcpSocket *c);
    void dropClient(QTcpSocket *c, const QString &reason);
    void removeClient(QTcpSocket *c);
    bool sendLine(QTcpSocket *c, const QString &text);
    int  comboIndexOf(QTcpSocket *c) const;
    void updateClients();

    QTcpServer  *server;
    QList<QTcpSocket*> clients_;
    QHash<QTcpSocket*, QString> names_;

    QSpinBox    *portSpin;
    QPushButton *startBtn;
    QPushButton *sendBtn;
    QLabel      *countLabel;
    QTextEdit   *log;
    QLineEdit   *input;
    QComboBox   *targetCombo;
};
