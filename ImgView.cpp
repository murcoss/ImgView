#include <QGuiApplication>
#include <QImageReader>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QThreadPool>
#include <QMimeData>
#include <QDirIterator>
#include <QInputDialog>
#include <QtConcurrent>

#include "ImgView.h"
#include "IconEngine.h"

void ImgLoaderTask::run(){
    QElapsedTimer ti;
    ti.start();
    if (imgStruct) {
        imgStruct->mutex.lock();
        QSet<ImgStruct::WorkToDo> worktodo = imgStruct->worktodo;
        QFileInfo const fi = imgStruct->fi;
        imgStruct->mutex.unlock();
        QImage img;
        if (worktodo.contains(ImgStruct::WorkToDo::loadImage)) {
            worktodo.insert(ImgStruct::WorkToDo::createThumbnail);
            QImageReader reader(fi.absoluteFilePath());
            
            bool const readok = reader.read(&img);
            QMutexLocker locker(&imgStruct->mutex);
            if (readok) {
                imgStruct->img = std::move(QPixmap::fromImage(img));
                imgStruct->size = img.size();
            } else {
                imgStruct->errormessage = reader.errorString();
            }
        }
        if (worktodo.contains(ImgStruct::WorkToDo::createThumbnail)) {
            if (img.isNull()){
                QImageReader reader(fi.absoluteFilePath());

                bool const readok = reader.read(&img);
                if (!readok) {
                    QMutexLocker locker(&imgStruct->mutex);
                    imgStruct->errormessage = reader.errorString();
                }
                emit loaded(ImgStruct::WorkResult::loadedImage, imgStruct->idx);
            }
            if (!img.isNull()) {
                qDebug() << "Loaded thumb idx " << imgStruct->idx;
                int const constexpr maxsize = 256;
                int const imgsize = std::max(img.width(), img.height());
                QPixmap px;
                if (imgsize < maxsize) {
                    px = QPixmap::fromImage(img);
                } else {
                    QImage const scaled = img.scaled(QSize(maxsize, maxsize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    px = QPixmap::fromImage(scaled);
                }
                QMutexLocker locker(&imgStruct->mutex);
                imgStruct->thumbnail = std::move(px);
                imgStruct->size = img.size();

                emit loaded(ImgStruct::WorkResult::loadedThumb, imgStruct->idx);
            }
        }
        if (imgStruct->worktodo.contains(ImgStruct::WorkToDo::destroyImage)) {
            QMutexLocker locker(&imgStruct->mutex);
            if (!imgStruct->img.isDetached()) {
                qDebug() << "pixmap about to be clear has still references!";
            }
            QPixmap().swap(imgStruct->img);
            emit loaded(ImgStruct::WorkResult::destroyedImage, imgStruct->idx);
        }
        QMutexLocker locker(&imgStruct->mutex);
        imgStruct->worktodo.clear();
        locker.unlock();
    }
    qDebug() << "ImgLoaderTask: " << ti.elapsed() << "ms";
}

void DirIteratorTask::run(){
    QElapsedTimer ti;
    ti.start();

    QList<ImgStruct*> tmpstruct;

    if (m_fns.isEmpty()){
        return;
    }

    //Read file list as a list of file
    QString filetoshowfirst;
    int idx = 0;
    for (auto const& fn : m_fns) {
        QFileInfo fi(fn);
        if (ImgView::supportedExtensions().contains(fi.suffix().toLower())) {
            tmpstruct.push_back(new ImgStruct);
            tmpstruct.back()->fn = fn;
            tmpstruct.back()->fi = fi;
            tmpstruct.back()->idx = idx++;
            filetoshowfirst = fn;
        }
    }

    if (!tmpstruct.isEmpty()) {
        emit loadedFilenames(tmpstruct);
        tmpstruct.clear();
    }

    //If we got a list of files, only load these
    if ((m_fns.size() > 1) && !QFileInfo(m_fns.front()).isDir()) {
        return;
    }

    //If it is a directory or only one file, iterate the directory
    QFileInfo fi(m_fns.front());
    QString dir = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
    QDirIterator it(dir, QDir::Files, m_itf);
    while (it.hasNext()) {
        QString const file = it.next();
        if (file != filetoshowfirst) {
            QFileInfo fi(file);
            if (ImgView::supportedExtensions().contains(fi.suffix().toLower())) {
                tmpstruct.push_back(new ImgStruct);
                tmpstruct.back()->fn = file;
                tmpstruct.back()->fi = fi;
                tmpstruct.back()->idx = idx++;
            }
        }

        if ((ti.elapsed() > 10) || !it.hasNext()) {
            ti.restart();
            emit loadedFilenames(tmpstruct);
            tmpstruct.clear();
        }
    }
}

ImgView::ImgView(QWidget* parent)
{
    setMouseTracking(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setAcceptDrops(true);

    connect(this, &ImgView::customContextMenuRequested, this, &ImgView::customContextMenu);

    QPushButton* btnLoad = new QPushButton("📂", this);
    btnLoad->setToolTip(QStringLiteral(u"Load Image"));
    btnLoad->setFixedSize(24, 24);
    btnLoad->raise();
    m_buttons.push_back(btnLoad);
    connect(btnLoad, &QPushButton::clicked, [this]() {
        loadImage(QStringList());
    });

    QPushButton* btnOpen = new QPushButton("📂", this);
    btnOpen->setToolTip(QStringLiteral(u"Open Folder"));
    btnOpen->setFixedSize(24, 24);
    btnOpen->raise();
    m_buttons.push_back(btnOpen);
    connect(btnOpen, &QPushButton::clicked, [this]() {
        openFolder(QString());
    });

    QPushButton* btnFit = new QPushButton("⤢", this);
    btnFit->setToolTip(QStringLiteral(u"Autofit"));
    btnFit->setFixedSize(24, 24);
    btnFit->raise();
    m_buttons.push_back(btnFit);
    connect(btnFit, &QPushButton::clicked, this, &ImgView::autofit);

    QPushButton* btnNext = new QPushButton("⏭️", this);
    btnNext->setToolTip(QStringLiteral(u"Load next Image"));
    btnNext->setFixedSize(24, 24);
    btnNext->raise();
    m_buttons.push_back(btnNext);
    connect(btnNext, &QPushButton::clicked, [this]() {
        nextImage(FileDir::next);
    });

    QPushButton* btnPrev = new QPushButton("⏮️", this);
    btnPrev->setToolTip(QStringLiteral(u"Load previous Image"));
    btnPrev->setFixedSize(24, 24);
    btnPrev->raise();
    m_buttons.push_back(btnPrev);
    connect(btnPrev, &QPushButton::clicked, [this]() {
        nextImage(FileDir::previous);
    });

    QPushButton* btnThumb = new QPushButton("🖼", this);
    btnThumb->setToolTip(QStringLiteral(u"Load previous Image"));
    btnThumb->setFixedSize(24, 24);
    btnThumb->raise();
    m_buttons.push_back(btnThumb);
    connect(btnThumb, &QPushButton::clicked, [this]() {
        m_show_thumb = !m_show_thumb;
        update();
    });

    QSettings settings("ImgView", "ImgView");
    m_wheel_zoom = settings.value("Wheel zoom").toBool();
    QPushButton* btnWheelFunction = new QPushButton(m_wheel_zoom ? QStringLiteral(u"🔍") : QStringLiteral(u"🔃"), this);
    btnWheelFunction->setToolTip(QStringLiteral(u"Wheel function"));
    btnWheelFunction->setFixedSize(24, 24);
    btnWheelFunction->raise();
    m_buttons.push_back(btnWheelFunction);
    connect(btnWheelFunction, &QPushButton::clicked, [=,this]() {
        m_wheel_zoom = !m_wheel_zoom;
        btnWheelFunction->setText(m_wheel_zoom ? QStringLiteral(u"🔍") : QStringLiteral(u"🔃"));
        QSettings settings("ImgView", "ImgView");
        settings.setValue("Wheel zoom", m_wheel_zoom);
        });

    QByteArrayList const bal = QImageReader::supportedImageFormats();
    m_supported_extensions.clear();
    for (auto const& b : bal) {
        m_supported_extensions.insert(b.toLower());
    }
    if (m_supported_extensions.contains("svg")){
        m_supported_extensions.remove("svg");
    }
    if (m_supported_extensions.contains("ico")) {
        m_supported_extensions.remove("ico");
    }
};

ImgView::~ImgView() {
};

void ImgView::paintEvent(QPaintEvent*){
    QElapsedTimer timer;
    timer.start();
    QPainter p(this);

    if (!m_imgstruct) {
        //just draw logo and quit
        IconEngine ie;
        QSize const si(160, 160);
        QRect const rect(QPoint(width() / 2 - si.width() / 2, height() / 2 - si.height() / 2), si);
        ie.paint(&p, rect, QIcon::Mode::Normal, QIcon::State::On);
        return;
    }

    ImgStruct &i = *m_imgstruct; 
    QMutexLocker locker(&i.mutex);
    bool showThumb = false;
    if (i.img.isNull()) {
        showThumb = true;
        if (i.thumbnail.isNull()) {
            QString const err = i.errormessage.isEmpty() ? QStringLiteral(u"Loading...") : i.errormessage;
            p.drawText(rect(), Qt::AlignCenter, err);
            return;
        }
    }

    QPixmap const& img = showThumb ? i.thumbnail : i.img;

    p.setTransform(m_transform);

    QSize si = size();
    qreal dpr = devicePixelRatioF();
    qreal dprs = devicePixelRatioFScale();

    QSizeF const scaledSize = i.size.scaled(size(), Qt::KeepAspectRatio);

    QPointF const topLeft((width() - scaledSize.width()) / 2.0,(height() - scaledSize.height()) / 2.0);
    QRectF const targetRect(topLeft, scaledSize);
    p.drawPixmap(targetRect, img, QRectF(QPointF(0, 0), img.size()));

    qDebug() << "Paintevent time: " << timer.nsecsElapsed() / 1000;
}

void ImgView::mouseDoubleClickEvent(QMouseEvent*){
    autofit();
}

void ImgView::autofit(){
    m_zoom = 1.;
    m_offset = { 0, 0 };
    setTransform();
    update();
}

void ImgView::mousePressEvent(QMouseEvent* event){
    m_lastMousePos = event->pos();
    event->accept();
}

void ImgView::mouseMoveEvent(QMouseEvent* event){
    if (event->buttons() & Qt::LeftButton) {
        QPoint const delta = event->pos() - m_lastMousePos;
        m_offset += delta;
        m_lastMousePos = event->pos();
        setTransform();
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void ImgView::mouseReleaseEvent(QMouseEvent* event){
    m_lastMousePos = QPoint(-1, -1);
    QWidget::mouseReleaseEvent(event);
}

void ImgView::wheelEvent(QWheelEvent* event) {
    int const angle = event->angleDelta().y();
    if (angle != 0) {
        bool const ctrl = QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier);
        if (ctrl == m_wheel_zoom) {
            FileDir const fd = angle > 0 ? FileDir::previous : FileDir::next;
            nextImage(fd);
        } else {
            double const factor = std::pow(1.2, angle / 120.);
            QPointF const mousePos = event->position();
            QPointF const beforeScale = (mousePos - m_offset - QPointF(width() / 2., height() / 2.)) / m_zoom;
            m_zoom *= factor;
            m_zoom = std::max(0.01, std::min(10000., m_zoom));
            m_offset = mousePos - (beforeScale * m_zoom) - QPointF(width() / 2., height() / 2.);
            setTransform();
            update();
        }
    }
    event->accept();
}

void ImgView::leaveEvent(QEvent*){
    m_lastMousePos = QPoint(-1, -1);
    update();
}

void ImgView::resizeEvent(QResizeEvent* event){
    int idx = 0;
    int const constexpr padding = 3;
    int height = 32;
    for (auto b : m_buttons) {
        int const buttonX = width() - b->width() - padding;
        int const buttonY = padding + idx * (b->height() + padding);

        b->move(buttonX, buttonY);
        b->raise();

        idx++;
        height = padding + idx * (b->height() + padding);
    }
    setMinimumHeight(height);
    setMinimumWidth(2 * height);

    setTransform();

    QWidget::resizeEvent(event);
}

void ImgView::keyPressEvent(QKeyEvent* event){
    if (event->key() == Qt::Key_PageDown || event->key() == Qt::Key_Right || event->key() == Qt::Key_Space) {
        nextImage(FileDir::next);
        event->accept();
    } else if (event->key() == Qt::Key_PageUp || event->key() == Qt::Key_Left) {
        nextImage(FileDir::previous);
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void ImgView::loadImage(QStringList filenames)
{
    QSettings settings("ImgView", "ImgView");

    // If we get no list of filenames, ask user
    if (filenames.isEmpty()) {
        QString lastDir = settings.value("LastDirectory", "").toString();
        QString supported(QStringLiteral(u"Image Files ("));
        for (auto const& s : m_supported_extensions) {
            supported += "*." + s + ' ';
        }
        supported += ')';
        filenames.push_back(QFileDialog::getOpenFileName(this, tr("Load Image File"), lastDir, supported));
        if (filenames.isEmpty()) {
            return;
        }
        if (!m_supported_extensions.contains(QFileInfo(filenames.front()).suffix().toLower())) {
            emit message(QStringLiteral(u"Not supported '%1'").arg(filenames.front()));
            return;
        }
        settings.setValue("LastDirectory", QFileInfo(filenames.back()).absolutePath());
    }

    getFiles(filenames, QDirIterator::Subdirectories);
}
void ImgView::getFiles(QStringList filenames, QDirIterator::IteratorFlag itf){
    clearImages();
    DirIteratorTask* dit = new DirIteratorTask(filenames, itf);
    connect(dit, &DirIteratorTask::loadedFilenames, this, &ImgView::loadedFilenames);
    QThreadPool::globalInstance()->start(dit);
}

void ImgView::openFolder(QString dir){
    if (dir.isEmpty()) {
        QSettings settings("ImgView", "ImgView");
        QString lastDir = settings.value("LastDirectory", "").toString();
        dir = QFileDialog::getExistingDirectory(this, QStringLiteral(u"Open Folder"), lastDir);
    }

    if (!dir.isEmpty()) {
        getFiles(QStringList { dir }, QDirIterator::Subdirectories);
    }
}

void ImgView::loaded(ImgStruct::WorkResult wr, size_t idx){
    if (wr == ImgStruct::WorkResult::loadedImage) { 
    if (m_imgstruct) {
            if (idx == m_imgstruct->idx) {
                update();
                setTitle();
            }
        }
    } else if (wr == ImgStruct::WorkResult::loadedThumb) {
        m_thumbcount++;
    }
    //Check if we have to load other files
    nextImage(ImgView::FileDir::none);
}

void ImgView::loadedFilenames(QList<ImgStruct*> is){
    if (is.isEmpty()){
        return;
    }
    if (m_allImages.isEmpty()) {
        m_imgstruct = is.front();
        update();
    }
    m_allImages.append(is);

    //Calculate Position in grid
    m_thumbcount = 0;
    int const dim = std::ceil(std::sqrt(m_allImages.size()));
    int x = 0, y = 0;
    for (auto& i : m_allImages){
        i->grid_idx = QPoint(x, y);
        QSizeF const scaledSize = i->size.scaled(QSizeF(1., 1.), Qt::KeepAspectRatio);
        i->grid_rect = QRectF(QPointF(x, y), scaledSize);
        x++;
        if (x >= dim){
            x = 0;
            y++;
        }
        if (i->thumbnail_loaded){
            m_thumbcount++;
        }
    }

    nextImage(ImgView::FileDir::none);
}

int mapIdxToRange(int idx, int range){
    if (idx < 0){
        idx += range;
    } else if (idx >= range) {
        idx -= range;
    }
    return idx;
}

void ImgView::nextImage(FileDir fd){

    if (m_allImages.isEmpty()) {
        return;
    }

    if (fd != FileDir::none) {
        int nextidx = m_imgstruct->idx;
        if (fd == FileDir::next) {
            nextidx++;
            if (nextidx >= m_allImages.size()) {
                nextidx -= m_allImages.size();
            }
        } else if (fd == FileDir::previous) {
            nextidx--;
            if (nextidx < 0) {
                nextidx += m_allImages.size();
            }
        }
        m_imgstruct = m_allImages.at(nextidx);
        autofit();
    } else {
        update();
    }

    //Set Title to new filename
    setTitle();

    // Limit loader threads
    int const cores = QThread::idealThreadCount();
    if (ImgLoaderTask::runningCount() >= cores) {
        qDebug() << "Too many threads!";
        return;
    }

    // Cache next Images
    int const current_image = m_imgstruct->idx;
    for (int i_ = current_image - cores-1; i_<current_image + cores+1; i_++){
        int i = i_;
        if (i < 0){
            i += m_allImages.size();
        } else if (i >= m_allImages.size()) {
            i -= m_allImages.size();    
        }
        if (i >= 0 && i < static_cast<int>(m_allImages.size())) {
            ImgStruct* is = m_allImages.at(i);
            if (is->mutex.tryLock()) {
                if (is->worktodo.isEmpty()) {
                    int dist = std::abs(i - current_image);
                    dist = std::min(dist, (int)m_allImages.size() - dist);
                    if (dist < cores) {
                        if (is->img.isNull()) {
                            is->worktodo.insert(ImgStruct::WorkToDo::loadImage);
                            qDebug() << "Load " << is->idx << " (" << ImgLoaderTask::runningCount() << ")";
                        }
                    } else {
                        if (!is->img.isNull()) {
                            is->worktodo.insert(ImgStruct::WorkToDo::destroyImage);
                        }
                    }
                    if (!is->worktodo.isEmpty()) {
                        ImgLoaderTask* ilt = new ImgLoaderTask(is);
                        connect(ilt, &ImgLoaderTask::loaded, this, &ImgView::loaded);
                        QThreadPool::globalInstance()->start(ilt);
                    }
                }
                is->mutex.unlock();
            }
        }
    }

    if (ImgLoaderTask::runningCount() < cores) {
        for (auto const& is : m_allImages){
            if (!is->load_thumbnail){
                if (is->mutex.tryLock()) {
                    is->load_thumbnail = true;
                    is->worktodo.insert(ImgStruct::WorkToDo::createThumbnail);
                    ImgLoaderTask* ilt = new ImgLoaderTask(is);
                    connect(ilt, &ImgLoaderTask::loaded, this, &ImgView::loaded);
                    QThreadPool::globalInstance()->start(ilt);
                    is->mutex.unlock();
                    if (ImgLoaderTask::runningCount() >= cores) {
                        break;
                    }
                }
            }
        }
    }

}

void ImgView::setTitle(){
    if (m_imgstruct) {
        ImgStruct & is = *m_imgstruct;
        emit message(QStringLiteral(u"%1 %2 (%3/%4/%5) (%6x%7 %8kB)")
                .arg(is.fi.fileName())
                .arg(is.fi.absoluteFilePath())
                .arg(is.idx + 1)
                .arg(m_thumbcount)
                .arg(m_allImages.size())
                .arg(is.size.width())
                .arg(is.size.height())
                .arg(is.fi.size() / (1024)));
    }
}

void ImgView::clearImages(){
    QtConcurrent::blockingMap(m_allImages, [](ImgStruct* img) {
        if (img) {
            img->mutex.lock();
            delete img;
        }
    });
    m_allImages.clear();
    m_imgstruct = 0;
}

void ImgView::setTransform(){
    m_transform.reset();
    QPoint const center(width() / 2, height() / 2);
    m_transform.translate(m_offset.x() + center.x(), m_offset.y() + center.y());
    m_transform.scale(m_zoom, m_zoom);
    m_transform.translate(-center.x(), -center.y());
}

void ImgView::customContextMenu(QPoint pos){
    QMenu* menu = new QMenu(this);
    menu->addAction(QStringLiteral(u"Load Image"), [this]() { loadImage(QStringList()); });
    menu->popup(mapToGlobal(pos));
}

void ImgView::dragEnterEvent(QDragEnterEvent* event){
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction(); // akzeptiere Drag & Drop
    }
}

void ImgView::dropEvent(QDropEvent* event){
    QList<QUrl> const droppedUrls = event->mimeData()->urls();
    QStringList filenames;
    for (QUrl const& url : droppedUrls) {
        filenames.push_back(url.toLocalFile());
    }
    if (!filenames.isEmpty()){
        loadImage(filenames);
    }
}