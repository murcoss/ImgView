#include "imagehashstore.h"
#include <QBuffer>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QMutexLocker>
#include <QDir>

ImageHashStore::ImageHashStore(){
    QMutexLocker locker(&lock);
    QString filename = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(filename);

    if (!dir.exists()) {
        dir.mkpath("."); // Create the full path if it doesn't exist
    }

    filename.append(QStringLiteral(u"/thumbs.db"));

    db = QSqlDatabase::addDatabase("QSQLITE", "ImageHashStoreConnection");
    db.setDatabaseName(filename);
    if (!db.open()) {
        qDebug() << db.lastError().nativeErrorCode();
        qFatal("Failed to open database.");
    }
    initializeTable();
}

ImageHashStore::~ImageHashStore(){
    QMutexLocker locker(&lock);
    if (db.isOpen()) {
        db.close();
    }
}

void ImageHashStore::initializeTable(){
    QSqlQuery query(db);
    if (!query.exec("CREATE TABLE IF NOT EXISTS images ("
                    "hash BLOB PRIMARY KEY,"
                    "image BLOB)")) {
        qWarning() << "Create table failed:" << query.lastError().text();
    }
}

QByteArray ImageHashStore::calculateHash(QByteArray const& image) {
    QCryptographicHash cryptoHash(QCryptographicHash::Sha256);
    cryptoHash.addData(image);
    return cryptoHash.result();
}

void ImageHashStore::insert(const QImage& thumb, QByteArray const& hash){
    QMutexLocker locker(&lock);
    QByteArray buffer;
    QBuffer qbuffer(&buffer);
    qbuffer.open(QIODevice::WriteOnly);
    thumb.save(&qbuffer, "JPEG", 100);
    qbuffer.close();

    QSqlQuery query(db);
    query.prepare("INSERT INTO images (hash, image) VALUES (:hash, :image)");
    query.bindValue(":hash", hash);
    query.bindValue(":image", buffer);

    if (!query.exec()) {
        qWarning() << "Insert failed:" << query.lastError().text();
    }

    return;
}

QImage ImageHashStore::get(const QByteArray& hash) {
    QMutexLocker locker(&lock);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(u"SELECT image FROM images WHERE hash = :hash"));
    query.bindValue(":hash", hash);
    if (!query.exec()) {
        qWarning() << "Get query failed:" << query.lastError().text();
        return QImage();
    }

    if (query.next()) {
        QByteArray imgData = query.value(0).toByteArray();
        QImage image;
        image.loadFromData(imgData, "JPEG");
        return image;
    }

    return QImage();
}
