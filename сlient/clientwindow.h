#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QSpinBox;
class QTcpSocket;
class QTextEdit;
class QTimer;

class ClientWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ClientWindow(QWidget *parent = nullptr);

private slots:
    void toggleConnection();
    void readData();
    void onError();
    void sendMessage();
    void onConnectionTimeout();

private:
    QTcpSocket  *socket;
    QTimer      *connectTimer;
    QLineEdit   *hostEdit;
    QLineEdit   *input;
    QSpinBox    *portSpin;
    QPushButton *connectBtn;
    QPushButton *sendBtn;
    QTextEdit   *log;

    void setDisconnectedState();
    void setConnectedState(const QString &connectionInfo);
};
