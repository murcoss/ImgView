#pragma once
#include <QFileInfo>
#include <QPointer>
#include <QString>

struct WorkItem {
    QFileInfo fi;
    bool loadthumb = false;
    bool loadimage = false;
    bool destroyimage = false;
    QString m_error_message;
    QByteArray m_hash;
};
