#pragma once

#include <QByteArray>
#include <QCryptographicHash>
#include <QImage>
#include <QSqlDatabase>
#include <QBuffer>
#include <QMutex>

class ImageHashStore {
public:
    ImageHashStore();
    ~ImageHashStore();

    void insert(QImage const& image, QByteArray const& hash);
    QImage get(const QByteArray& hash);
    static QByteArray calculateHash(QByteArray const& image);

private:
    void initializeTable();

    QSqlDatabase db;
    QMutex lock;
};