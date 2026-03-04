#include "DirIteratorTask.h"
#include <QElapsedTimer>

void DirIteratorTask::run()
{
    QElapsedTimer ti;
    ti.start();

    QList<ImageItem*> newimageitems;

    if (m_fns.isEmpty()) {
        return;
    }

    // Read file list as a list of file
    QString filetoshowfirst;
    for (auto const& fn : m_fns) {
        QFileInfo const fi(fn);
        if (supportedExtensions().contains(fi.suffix().toLower())) {
            newimageitems.push_back(new ImageItem(fi));
            filetoshowfirst = fn;
        }
    }

    if (!newimageitems.isEmpty()) {
        emit loadedFilenames(newimageitems);
        newimageitems.clear();
    }

    // If we got a list of files, only load these
    if ((m_fns.size() > 1) && !QFileInfo(m_fns.front()).isDir()) {
        return;
    }

    // If it is a directory or only one file, iterate the directory
    QFileInfo fi(m_fns.front());
    QString dir = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
    QDirIterator it(dir, QDir::Files, m_itf);
    while (it.hasNext()) {
        QString const file = it.next();
        if (file != filetoshowfirst) {
            QFileInfo const fi(file);
            if (DirIteratorTask::supportedExtensions().contains(fi.suffix().toLower())) {
                newimageitems.push_back(new ImageItem(fi));
            }
        }

        if ((ti.elapsed() > 10) || !it.hasNext()) {
            ti.restart();
            emit loadedFilenames(newimageitems);
            newimageitems.clear();
        }
    }
}