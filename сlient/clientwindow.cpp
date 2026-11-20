// окно клиента
#include "clientwindow.h"
#include "../protocol.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTcpSocket>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QTimer>

namespace {

constexpr int kConnectTimeoutMs = 5000;          // таймаут подключения к серверу
constexpr auto kClientTitle = "TCP Клиент";

}

ClientWindow::ClientWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(kClientTitle);
    setMinimumSize(420, 360);

    hostEdit = new QLineEdit("127.0.0.1");
    portSpin = new QSpinBox;
    portSpin->setRange(1024, 65535);
    portSpin->setValue(12345);
    connectBtn = new QPushButton("Подключиться");

    log = new QTextEdit;
    log->setReadOnly(true);

    input = new QLineEdit;
    input->setPlaceholderText("Сообщение серверу...");
    sendBtn = new QPushButton("Отправить");
    sendBtn->setEnabled(false);

    auto *top = new QHBoxLayout;
    top->addWidget(new QLabel("Хост:"));
    top->addWidget(hostEdit);
    top->addWidget(new QLabel("Порт:"));
    top->addWidget(portSpin);
    top->addWidget(connectBtn);

    auto *bottom = new QHBoxLayout;
    bottom->addWidget(input);
    bottom->addWidget(sendBtn);

    auto *main = new QVBoxLayout(this);
    main->addLayout(top);
    main->addWidget(log);
    main->addLayout(bottom);

    socket = new QTcpSocket(this);

    // таймер для ошибки - долгое подключение к серверу
    connectTimer = new QTimer(this);
    connectTimer->setSingleShot(true);
    connect(connectTimer, &QTimer::timeout, this, &ClientWindow::onConnectionTimeout);

    connect(connectBtn, &QPushButton::clicked, this, &ClientWindow::toggleConnection);
    connect(sendBtn, &QPushButton::clicked, this, &ClientWindow::sendMessage);
    connect(input, &QLineEdit::returnPressed, this, &ClientWindow::sendMessage);

    // обработка успешного подключения к серверу
    connect(socket, &QTcpSocket::connected, this, [this]() {
        connectTimer->stop();
        log->append("Подключено к серверу");

        QString myIp = socket->localAddress().toString();
        if (myIp.startsWith("::ffff:"))
            myIp = myIp.mid(7);

        setConnectedState(myIp + ":" + QString::number(socket->localPort()));
    });

    // обработка отключения от сервера
    connect(socket, &QTcpSocket::disconnected, this, [this]() {
        log->append("Отключено от сервера");
        setDisconnectedState();
    });

    connect(socket, &QTcpSocket::readyRead, this, &ClientWindow::readData);

    // обработка разрыва связи с сервером
    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        connect(socket, &QAbstractSocket::errorOccurred, this, &ClientWindow::onError);
    #else
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
                this, &ClientWindow::onError);
    #endif

}

// вкл/выкл соединение соединения с сервером
void ClientWindow::toggleConnection()
{
    if (socket->state() == QAbstractSocket::UnconnectedState) {
        connectBtn->setEnabled(false);
        connectBtn->setText("Подключение...");

        connectTimer->start(kConnectTimeoutMs);
        socket->connectToHost(hostEdit->text(), portSpin->value());
    } else {
        socket->disconnectFromHost();
    }
}

// чтение сообщения от сервера с обработкой ошибки - большое сообщение
void ClientWindow::readData()
{
    while (socket->canReadLine()) {
        const QByteArray raw = socket->readLine(Protocol::kReadLimit);
        if (!raw.endsWith('\n')) {
            log->append("Ошибка: слишком длинное сообщение от сервера, соединение закрыто");
            socket->abort();
            return;
        }
        log->append(Protocol::decodeLine(raw));
    }
    if (socket->bytesAvailable() > Protocol::kReadLimit) {
        log->append("Ошибка: слишком длинное сообщение от сервера, соединение закрыто");
        socket->abort();
    }
}

// обработка ошибки сетевого соединения
void ClientWindow::onError()
{
    connectTimer->stop();
    log->append("Ошибка: " + socket->errorString());
    if (socket->state() != QAbstractSocket::ConnectedState) {
        setDisconnectedState();
    }
}

// обработка отпарвки сообщения серверу
void ClientWindow::sendMessage()
{
    const QString text = input->text().trimmed();
    if (text.isEmpty() || socket->state() != QAbstractSocket::ConnectedState)
        return;

    if (text.toUtf8().size() > Protocol::kMaxLineBytes) {
        log->append(QString("Сообщение слишком длинное (максимум %1 байт)").arg(Protocol::kMaxLineBytes));
        return;
    }

    const QByteArray data = Protocol::encodeLine(text);
    if (socket->write(data) != data.size()) {
        log->append("Не удалось отправить сообщение: " + socket->errorString());
        return;
    }
    log->append("Я: " + text);
    input->clear();
}

// обработка ошибки долго подключения к серверу
void ClientWindow::onConnectionTimeout()
{
    if (socket->state() == QAbstractSocket::ConnectingState) {
        socket->abort();
        log->append(QString("Ошибка: Превышено время ожидания подключения (%1 сек)").arg(kConnectTimeoutMs / 1000));
        setDisconnectedState();
    }
}

// возрат интерфейса к исходному состоянию
void ClientWindow::setDisconnectedState()
{
    setWindowTitle(kClientTitle);
    connectBtn->setText("Подключиться");
    connectBtn->setEnabled(true);
    sendBtn->setEnabled(false);
}

// перевод интерфейса в состояние подключения
void ClientWindow::setConnectedState(const QString &connectionInfo)
{
    setWindowTitle(QString("%1 [%2]").arg(kClientTitle, connectionInfo));
    connectBtn->setText("Отключиться");
    connectBtn->setEnabled(true);
    sendBtn->setEnabled(true);
}