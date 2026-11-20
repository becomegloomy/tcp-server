// общие константы и функции
#pragma once

#include <QString>
#include <QByteArray>

namespace Protocol {

    constexpr qint64 kMaxLineBytes = 4096;           // макс длина сообщения 
    constexpr qint64 kReadLimit = kMaxLineBytes + 3; // лимит чтения сокета

    // раскодирование сообщения
    inline QString decodeLine(QByteArray l)
    {
        if (l.endsWith('\n')) l.chop(1);
        if (l.endsWith('\r')) l.chop(1);
        return QString::fromUtf8(l);
    }

    // кодирование сообщения
    inline QByteArray encodeLine(const QString &text)
    {
        return (text + "\n").toUtf8();
    }
}