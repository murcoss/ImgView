#include "imagehashstore.h"
#include <QBuffer>
#include <QDebug>
#include <QSqlError>
#include <QStandardPaths>
#include <QMutexLocker>
#include <QDir>

ImageHashStore::ImageHashStore(){
    QMutexLocker locker(&lock);
    QString filename = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(filename);

    if (!dir.exists()) {
        dir.mkpath(".");
    }

    filename.append(QStringLiteral(u"/thumbs.db"));

    db = QSqlDatabase::addDatabase("QSQLITE", "ImageHashStoreConnection");
    db.setDatabaseName(filename);
    if (!db.open()) {
        qDebug() << db.lastError().nativeErrorCode();
        return;
    }

    QSqlQuery query(db);
    if (!query.exec("CREATE TABLE IF NOT EXISTS images (hash BLOB PRIMARY KEY, image BLOB, filepath TEXT, filesize INTEGER)")) {
        qWarning() << "Create table failed:" << query.lastError().text();
        return;
    }

    m_insert_query = QSqlQuery(db);
    m_insert_query.prepare("INSERT INTO images (hash, image, filepath, filesize) VALUES (:hash, :image, :filepath, :filesize)");

    m_get_by_hash_query = QSqlQuery(db);
    m_get_by_hash_query.prepare(QStringLiteral(u"SELECT image FROM images WHERE hash = :hash"));
}

ImageHashStore::~ImageHashStore(){
    QMutexLocker locker(&lock);
    if (db.isOpen()) {
        db.close();
    }
}

//QByteArray ImageHashStore::calculateHash(QByteArray const& image) {
//    QCryptographicHash cryptoHash(QCryptographicHash::Sha256);
//    cryptoHash.addData(image);
//    return cryptoHash.result();
//}

void ImageHashStore::insert(QImage thumb, QByteArray const hash, QString const filepath, uint64_t filesize){
    QMutexLocker locker(&lock);
    QByteArray buffer;
    QBuffer qbuffer(&buffer);
    qbuffer.open(QIODevice::WriteOnly);
    thumb.save(&qbuffer, "JPEG", 100);

    //query.prepare("INSERT INTO images (hash, image, filepath, filesize) VALUES (:hash, :image, :filepath, :filesize)");
    m_insert_query.finish();
    m_insert_query.bindValue(":hash", hash);
    m_insert_query.bindValue(":image", buffer);
    m_insert_query.bindValue(":filepath", filepath);
    m_insert_query.bindValue(":filesize", static_cast<qint64>(filesize));

    if (!m_insert_query.exec()) {
        qWarning() << "Insert failed:" << m_insert_query.lastError().text();
    }

    return;
}

QByteArray ImageHashStore::getByHash(const QByteArray hash)
{
    QMutexLocker locker(&lock);

    m_get_by_hash_query.finish();
    m_get_by_hash_query.bindValue(":hash", hash);
    if (!m_get_by_hash_query.exec()) {
        return QByteArray();
    }

    if (m_get_by_hash_query.next()) {
        QByteArray const imgData = m_get_by_hash_query.value(0).toByteArray();
        return imgData;
    }

    return QByteArray();
}