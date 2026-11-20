// окно сервера
#include "serverwindow.h"
#include "../protocol.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

// генерация имени клиента IP+порт
QString peerName(QTcpSocket *c)
{
    QHostAddress addr = c->peerAddress();
    if (addr.isInSubnet(QHostAddress("::ffff:0:0"), 96))
        addr = QHostAddress(addr.toIPv4Address());
    return addr.toString() + ":" + QString::number(c->peerPort());
}

}

ServerWindow::ServerWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("TCP Сервер");
    setMinimumSize(500, 380);

    portSpin = new QSpinBox;
    portSpin->setRange(1024, 65535);
    portSpin->setValue(12345);
    portSpin->setMaximumWidth(120);
    startBtn = new QPushButton("Запустить");
    countLabel = new QLabel("Клиентов: 0");

    log = new QTextEdit;
    log->setReadOnly(true);

    // выпадающий список для выбора получателя сообщения (если клиентов несолько)
    targetCombo = new QComboBox;
    targetCombo->addItem("Всем", QVariant::fromValue<QTcpSocket*>(nullptr));
    targetCombo->setEnabled(false);

    input = new QLineEdit;
    input->setPlaceholderText("Сообщение...");
    sendBtn = new QPushButton("Отправить");
    sendBtn->setEnabled(false);

    auto *top = new QHBoxLayout;
    top->addWidget(new QLabel("Порт:"));
    top->addWidget(portSpin);
    top->addWidget(startBtn);
    top->addStretch();
    top->addWidget(countLabel);

    auto *bottom = new QHBoxLayout;
    bottom->addWidget(targetCombo);
    bottom->addWidget(input);
    bottom->addWidget(sendBtn);

    auto *main = new QVBoxLayout(this);
    main->addLayout(top);
    main->addWidget(log);
    main->addLayout(bottom);

    server = new QTcpServer(this);

    connect(startBtn, &QPushButton::clicked, this, &ServerWindow::toggleServer);
    connect(server, &QTcpServer::newConnection, this, &ServerWindow::onNewConnection);
    connect(sendBtn, &QPushButton::clicked, this, &ServerWindow::sendMessage);
    connect(input, &QLineEdit::returnPressed, this, &ServerWindow::sendMessage);
}

// вкл/выкл сервера
void ServerWindow::toggleServer()
{
    if (server->isListening()) {
        for (QTcpSocket *c : clients_)
            c->disconnectFromHost();
        server->close();
        startBtn->setText("Запустить");
        portSpin->setEnabled(true);
        updateClients();
        log->append("Сервер остановлен");
        return;
    }
    if (!server->listen(QHostAddress::Any, portSpin->value())) {
        log->append("Ошибка: " + server->errorString());
        return;
    }
    startBtn->setText("Остановить");
    portSpin->setEnabled(false);
    updateClients();
    log->append(QString("Сервер слушает порт %1").arg(server->serverPort()));
}

// обработка подключения нового клиента
void ServerWindow::onNewConnection()
{
    while (server->hasPendingConnections()) {
        QTcpSocket *c = server->nextPendingConnection();
        clients_.append(c);
        names_.insert(c, peerName(c));
        targetCombo->addItem(names_.value(c), QVariant::fromValue(c));

        log->append("Подключился клиент: " + names_.value(c));
        updateClients();

        connect(c, &QTcpSocket::readyRead, this, [this, c]() { readClient(c); });
        connect(c, &QTcpSocket::disconnected, this, [this, c]() { removeClient(c); });
    }
}

// чтение сообщения клиента с обработкой ошибки - большое сообщение
void ServerWindow::readClient(QTcpSocket *c)
{
    while (c->canReadLine()) {
        const QByteArray raw = c->readLine(Protocol::kReadLimit);
        if (!raw.endsWith('\n')) {
            dropClient(c, "слишком длинное сообщение");
            return;
        }
        const QString line = Protocol::decodeLine(raw);
        log->append("Клиент " + names_.value(c) + ": " + line);
    }

    if (c->bytesAvailable() > Protocol::kReadLimit) {
        dropClient(c, "слишком длинное сообщение без перевода строки");
    }
}

// жесткое удаление клиента (если попытался отправить большое сообщение)
void ServerWindow::dropClient(QTcpSocket *c, const QString &reason)
{
    log->append("Клиент " + names_.value(c) + " отключен принудительно: " + reason);
    c->abort();
    removeClient(c);
}

// обычное удаление клиента (прекращение связи)
void ServerWindow::removeClient(QTcpSocket *c)
{
    if (!clients_.removeOne(c))
        return;

    log->append("Клиент отключился: " + names_.value(c));
    names_.remove(c);

    int idx = comboIndexOf(c);
    if (idx != -1)
        targetCombo->removeItem(idx);

    c->deleteLater();
    updateClients();
}

// кодирование и отправка строки с контролем целостности
bool ServerWindow::sendLine(QTcpSocket *c, const QString &text)
{
    if (c->state() != QAbstractSocket::ConnectedState)
        return false;

    const QByteArray data = Protocol::encodeLine(text);
    return c->write(data) == data.size();
}

// формирование и отпарвка сообщения, выбор получателя/-ей
void ServerWindow::sendMessage()
{
    const QString text = input->text().trimmed();
    if (text.isEmpty() || clients_.isEmpty())
        return;

    if (text.toUtf8().size() > Protocol::kMaxLineBytes) {
        log->append(QString("Сообщение слишком длинное (максимум %1 байт)").arg(Protocol::kMaxLineBytes));
        return;
    }

    QTcpSocket *target = targetCombo->currentData().value<QTcpSocket*>();
    const QString fullMessage = "Сервер: " + text;

    if (!target) {
        int failed = 0;
        for (QTcpSocket *c : clients_)
            if (!sendLine(c, fullMessage))
                ++failed;
        log->append("Сервер (всем): " + text);
        if (failed)
            log->append(QString("Не удалось отправить: %1 клиент(ам)").arg(failed));
    } else {
        if (sendLine(target, fullMessage)) {
            log->append("Сервер -> " + names_.value(target) + ": " + text);
        } else {
            log->append("Не удалось отправить сообщение клиенту " + names_.value(target));
        }
    }
    input->clear();
}

// поиск клиента по индексу в выпадающем списке
int ServerWindow::comboIndexOf(QTcpSocket *c) const
{
    for (int i = 1; i < targetCombo->count(); ++i)
        if (targetCombo->itemData(i).value<QTcpSocket*>() == c)
            return i;
    return -1;
}

// обновление интерфейса в зависимости от состояния сервера
void ServerWindow::updateClients()
{
    countLabel->setText(QString("Клиентов: %1").arg(clients_.size()));

    const bool hasClients = server->isListening() && !clients_.isEmpty();
    sendBtn->setEnabled(hasClients);
    targetCombo->setEnabled(hasClients);

    auto *model = qobject_cast<QStandardItemModel*>(targetCombo->model());
    if (!model || !model->item(0))
        return;

    if (clients_.size() == 1) {
        targetCombo->setCurrentIndex(1);
        model->item(0)->setEnabled(false);
    } else {
        model->item(0)->setEnabled(true);
        if (clients_.isEmpty())
            targetCombo->setCurrentIndex(0);
    }
}