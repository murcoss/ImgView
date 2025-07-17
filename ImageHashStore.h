#pragma once
#include <QByteArray>
#include <QCryptographicHash>
#include <QImage>
#include <QSqlDatabase>
#include <QBuffer>
#include <QMutex>
#include <QSqlQuery>

class ImageHashStore {
public:
    ImageHashStore();
    ~ImageHashStore();

    void insert(QImage thumb, QByteArray const hash, QString const filepath, uint64_t filesize);
    QByteArray getByHash(const QByteArray hash);

private:
    QSqlDatabase db;
    QMutex lock;
    QSqlQuery m_insert_query, m_get_by_hash_query;
};