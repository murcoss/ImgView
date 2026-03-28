#include "ImageHashStore.h"
#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QMutexLocker>
#include <QSqlError>
#include <QStandardPaths>

ImageHashStore::ImageHashStore(QObject* parent)
    : QObject(parent) {
}

ImageHashStore::~ImageHashStore() {
    if (db.isOpen()) {
        db.close();
    }
}

void ImageHashStore::insertThumb(WorkItem wi, QByteArray buffer) {
    qDebug() << "saved thumb with " << buffer.size() << "bytes";

    m_insert_query.finish();
    m_insert_query.bindValue(":hash", wi.m_hash);
    m_insert_query.bindValue(":image", buffer);
    m_insert_query.bindValue(":filepath", wi.fi.filePath());
    m_insert_query.bindValue(":filesize", static_cast<qint64>(wi.fi.size()));

    if (!m_insert_query.exec()) {
        qWarning() << "Insert failed:" << m_insert_query.lastError().text();
    }

    return;
}

void ImageHashStore::requestThumb(WorkItem wi) {
    QByteArray imgData;
    QSize si;
    m_get_by_hash_query.finish();
    m_get_by_hash_query.bindValue(":hash", wi.m_hash);
    if (m_get_by_hash_query.exec()) {
        if (m_get_by_hash_query.next()) {
            imgData = m_get_by_hash_query.value(0).toByteArray();
            si.setWidth(m_get_by_hash_query.value(1).toInt());
            si.setHeight(m_get_by_hash_query.value(2).toInt());
        }
    }
    emit thumbReady(wi, QImage(imgData), si);
}

void ImageHashStore::init() {
    QString filename = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QDir().mkpath(filename);
    filename += "/thumbs.db";

    db = QSqlDatabase::addDatabase("QSQLITE", "ImageHashStoreConnection");
    db.setDatabaseName(filename);
    if (!db.open()) {
        qDebug() << db.lastError().nativeErrorCode();
        return;
    }

    QSqlQuery pragma(db);
    pragma.exec("PRAGMA journal_mode=WAL;");
    pragma.exec("PRAGMA synchronous=NORMAL;");

    QSqlQuery query(db);
    if (!query.exec("CREATE TABLE IF NOT EXISTS images (hash BLOB PRIMARY KEY, image BLOB, filepath TEXT, filesize INTEGER, width INTEGER, height INTEGER)")) {
        qWarning() << "Create table failed:" << query.lastError().text();
        return;
    }

    m_insert_query = QSqlQuery(db);
    m_insert_query.prepare("INSERT OR REPLACE INTO images VALUES (:hash, :image, :filepath, :filesize, :width, :height)");

    m_get_by_hash_query = QSqlQuery(db);
    m_get_by_hash_query.prepare(QStringLiteral(u"SELECT image, width, height FROM images WHERE hash = :hash"));
}
