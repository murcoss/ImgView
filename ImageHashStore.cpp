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
    if (!query.exec("CREATE TABLE IF NOT EXISTS images (hash BLOB PRIMARY KEY,image BLOB)")) {
        qWarning() << "Create table failed:" << query.lastError().text();
        return;
    }

    m_insert_query = QSqlQuery(db);
    query.prepare("INSERT INTO images (hash, image) VALUES (:hash, :image)");

    m_get_query = QSqlQuery(db);
    m_get_query.prepare(QStringLiteral(u"SELECT image FROM images WHERE hash = :hash"));
}

ImageHashStore::~ImageHashStore(){
    QMutexLocker locker(&lock);
    if (db.isOpen()) {
        db.close();
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

    m_insert_query.bindValue(":hash", hash);
    m_insert_query.bindValue(":image", buffer);

    if (!m_insert_query.exec()) {
        qWarning() << "Insert failed:" << m_insert_query.lastError().text();
    }

    return;
}

QImage ImageHashStore::get(const QByteArray& hash) {
    QMutexLocker locker(&lock);

    m_get_query.bindValue(":hash", hash);
    if (!m_get_query.exec()) {
        return QImage();
    }

    if (m_get_query.next()) {
        QByteArray imgData = m_get_query.value(0).toByteArray();
        QImage image;
        image.loadFromData(imgData, "JPEG");
        return image;
    }

    return QImage();
}
