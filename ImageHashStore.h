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

    void insert(QImage const& image, QByteArray const& hash);
    QImage get(const QByteArray& hash);
    static QByteArray calculateHash(QByteArray const& image);

private:
    QSqlDatabase db;
    QMutex lock;
    QSqlQuery m_insert_query, m_get_query;
};