#pragma once
#include <QCryptographicHash>
#include <QImage>
#include <QSqlDatabase>
#include <QBuffer>
#include <QMutex>
#include <QSqlQuery>

#include "WorkItem.h"

class ImageHashStore : public QObject{
    Q_OBJECT
public:
    explicit ImageHashStore(QObject* parent = nullptr);
    ~ImageHashStore();

public slots:
    void insertThumb(WorkItem wi, QByteArray thumbdata, QSize si);
    void requestThumb(WorkItem wi);
    void init();

signals:
    void thumbReady(WorkItem wi, QImage thumb, QSize si);

private:
    QSqlDatabase db;
    QSqlQuery m_insert_query, m_get_by_hash_query;
};
