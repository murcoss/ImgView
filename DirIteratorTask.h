#include <QObject>
#include <QString>
#include <QRunnable>
#include <QDirIterator>
#include <QImageReader>

#include "ImageItem.h"

class DirIteratorTask : public QObject, public QRunnable {
    Q_OBJECT

    QStringList m_fns;
    QDirIterator::IteratorFlag m_itf;

public:
    DirIteratorTask(QStringList fns, QDirIterator::IteratorFlag itf) : m_fns(fns), m_itf(itf){
        setAutoDelete(true);
    }

    static const QSet<QString>& supportedExtensions(){
        static QSet<QString> extensions = [] {
            QSet<QString> ext;
            for (const auto& b : QImageReader::supportedImageFormats()) {
                if (b != "svg" && b != "ico") {
                    ext.insert(QString::fromLatin1(b).toLower());
                }
            }
            return ext;
        }();
        return extensions;
    }

    void run() override;

signals:
    void loadedFilenames(QList<ImageItem*> list);
};
