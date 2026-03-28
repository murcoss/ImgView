#include <QDirIterator>
#include <QGuiApplication>
#include <QImageReader>
#include <QInputDialog>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QStyle>
#include <QThreadPool>
#include <QtConcurrent>
#include <qvariant.h>

#include "DirIteratorTask.h"
#include "ImgView.h"

ImgView::ImgView(QWidget *p)
    : QWidget(p) {
    setMouseTracking(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setAcceptDrops(true);

    QSettings settings("ImgView", "ImgView");

    connect(this, &ImgView::customContextMenuRequested, this, &ImgView::customContextMenu);

    QPushButton *btnLoad = new QPushButton(this);
    btnLoad->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    btnLoad->setToolTip(QStringLiteral(u"Load Image"));
    btnLoad->setFixedSize(24, 24);
    btnLoad->raise();
    m_buttons.push_back(btnLoad);
    connect(btnLoad, &QPushButton::clicked, [this]() { loadImage(QStringList()); });

    QPushButton *btnOpen = new QPushButton(this);
    btnOpen->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    btnOpen->setToolTip(QStringLiteral(u"Open Folder"));
    btnOpen->setFixedSize(24, 24);
    btnOpen->raise();
    m_buttons.push_back(btnOpen);
    connect(btnOpen, &QPushButton::clicked, [this]() { openFolder(QString()); });

    QPushButton *btnOpenDatabase = new QPushButton(QStringLiteral(u"🗃️"), this);
    btnOpenDatabase->setToolTip(QStringLiteral(u"Open Folder"));
    btnOpenDatabase->setFixedSize(24, 24);
    btnOpenDatabase->raise();
    m_buttons.push_back(btnOpenDatabase);
    connect(btnOpenDatabase, &QPushButton::clicked, this, &ImgView::openDatabase);

    m_antialiase = settings.value("Wheel zoom").toBool();
    QPushButton *btnSmooth = new QPushButton(
        m_antialiase ? QStringLiteral(u"🌀") : QStringLiteral(u"░"), this);
    btnSmooth->setToolTip(QStringLiteral(u"Autofit"));
    btnSmooth->setFixedSize(24, 24);
    btnSmooth->raise();
    m_buttons.push_back(btnSmooth);
    connect(btnSmooth, &QPushButton::clicked, [this, btnSmooth]() {
        m_antialiase = !m_antialiase;
        btnSmooth->setText(m_antialiase ? QStringLiteral(u"🌀") : QStringLiteral(u"░"));
        QSettings settings("ImgView", "ImgView");
        settings.setValue("Antialiase", m_antialiase);
        update();
    });

    QPushButton *btnFit = new QPushButton("⤢", this);
    btnFit->setToolTip(QStringLiteral(u"Autofit"));
    btnFit->setFixedSize(24, 24);
    btnFit->raise();
    m_buttons.push_back(btnFit);
    connect(btnFit, &QPushButton::clicked, this, &ImgView::autofit);

    QPushButton *btnNext = new QPushButton("⏭️", this);
    btnNext->setToolTip(QStringLiteral(u"Load next Image"));
    btnNext->setFixedSize(24, 24);
    btnNext->raise();
    m_buttons.push_back(btnNext);
    connect(btnNext, &QPushButton::clicked, [this]() { nextImage(FileDir::next); });

    QPushButton *btnPrev = new QPushButton("⏮️", this);
    btnPrev->setToolTip(QStringLiteral(u"Load previous Image"));
    btnPrev->setFixedSize(24, 24);
    btnPrev->raise();
    m_buttons.push_back(btnPrev);
    connect(btnPrev, &QPushButton::clicked, [this]() { nextImage(FileDir::previous); });

    QPushButton *btnThumb = new QPushButton("🖼", this);
    btnThumb->setToolTip(QStringLiteral(u"Load previous Image"));
    btnThumb->setFixedSize(24, 24);
    btnThumb->raise();
    m_buttons.push_back(btnThumb);
    connect(btnThumb, &QPushButton::clicked, [this]() {
        m_show_thumb = !m_show_thumb;
        update();
    });

    m_wheel_zoom = settings.value("Wheel zoom").toBool();
    QPushButton *btnWheelFunction = new QPushButton(
        m_wheel_zoom ? QStringLiteral(u"🔍") : QStringLiteral(u"🔃"), this);
    btnWheelFunction->setToolTip(QStringLiteral(u"Wheel function"));
    btnWheelFunction->setFixedSize(24, 24);
    btnWheelFunction->raise();
    m_buttons.push_back(btnWheelFunction);
    connect(btnWheelFunction, &QPushButton::clicked, [=, this]() {
        m_wheel_zoom = !m_wheel_zoom;
        btnWheelFunction->setText(m_wheel_zoom ? QStringLiteral(u"🔍") : QStringLiteral(u"🔃"));
        QSettings settings("ImgView", "ImgView");
        settings.setValue("Wheel zoom", m_wheel_zoom);
    });

    QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

    connect(&m_imageloaderqueue, &ImageLoaderQueue::requestReady, this, &ImgView::loadedImage);
};

ImgView::~ImgView() {};

void ImgView::paintEvent(QPaintEvent *) {
    QElapsedTimer timer;
    timer.start();
    QPainter p(this);

    if (m_antialiase) {
        p.setRenderHints(QPainter::RenderHint::SmoothPixmapTransform | QPainter::RenderHint::Antialiasing);
    }

    p.setTransform(m_transform);

    for (auto *ii : m_allImages) {
        ii->draw(p, m_mouselogicalpos);
    }

    // qDebug() << "Paintevent time us: " << timer.nsecsElapsed() / 1000;
}

void ImgView::mouseDoubleClickEvent(QMouseEvent *) { autofit(); }

void ImgView::autofit() {
    m_zoom = 1.;
    m_offset = { 0, 0 };
    setTransform();
    update();
}

void ImgView::mousePressEvent(QMouseEvent *event) {
    m_lastMousePos = event->pos();
    event->accept();
}

void ImgView::mouseMoveEvent(QMouseEvent *event) {
    m_mouselogicalpos = m_transform.inverted().map(event->pos().toPointF());

    if (event->buttons() & Qt::LeftButton) {
        QPointF const delta = event->pos() - m_lastMousePos;
        m_offset += delta;
        m_lastMousePos = event->pos();
        setTransform();
        update();
    }
    QWidget::mouseMoveEvent(event);
    update();
}

void ImgView::mouseReleaseEvent(QMouseEvent *event) {
    m_lastMousePos = QPoint(-1, -1);
    QWidget::mouseReleaseEvent(event);
}

void ImgView::wheelEvent(QWheelEvent *event) {
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

void ImgView::leaveEvent(QEvent *) {
    m_lastMousePos = QPoint(-1, -1);
    update();
}

void ImgView::resizeEvent(QResizeEvent *event) {
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

void ImgView::keyPressEvent(QKeyEvent *event) {
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

void ImgView::loadImage(QStringList filenames) {
    QSettings settings("ImgView", "ImgView");

    // If we get no list of filenames, ask user
    if (filenames.isEmpty()) {
        QString lastDir = settings.value("LastDirectory", "").toString();
        QString supported(QStringLiteral(u"Image Files ("));
        for (auto const &s : DirIteratorTask::supportedExtensions()) {
            supported += "*." + s + ' ';
        }
        supported += ')';
        filenames.push_back(QFileDialog::getOpenFileName(
            this, tr("Load Image File"), lastDir, supported));
        if (filenames.isEmpty()) {
            return;
        }
        if (!DirIteratorTask::supportedExtensions().contains(
                QFileInfo(filenames.front()).suffix().toLower())) {
            emit message(
                QStringLiteral(u"Not supported '%1'").arg(filenames.front()));
            return;
        }
        settings.setValue("LastDirectory",
                          QFileInfo(filenames.back()).absolutePath());
    }

    getFiles(filenames, QDirIterator::Subdirectories);
}

void ImgView::getFiles(QStringList filenames, QDirIterator::IteratorFlag itf) {
    clearImages();
    DirIteratorTask *dit = new DirIteratorTask(filenames, itf);
    connect(dit, &DirIteratorTask::loadedFilenames, this, &ImgView::loadedFilenames);
    QThreadPool::globalInstance()->start(dit);
}

void ImgView::openFolder(QString dir) {
    if (dir.isEmpty()) {
        QSettings settings("ImgView", "ImgView");
        QString lastDir = settings.value("LastDirectory", "").toString();
        dir = QFileDialog::getExistingDirectory(
            this, QStringLiteral(u"Open Folder"), lastDir, QFileDialog::DontUseNativeDialog);
    }

    if (!dir.isEmpty()) {
        getFiles(QStringList{ dir }, QDirIterator::Subdirectories);
    }
}

void ImgView::loaded(WorkItem) {
    update();

    // Check if we have to load other files
    nextImage(ImgView::FileDir::none);
}

void ImgView::loadedImage(WorkItem wi, QImage img, QImage thumb, QSize si) {
    for (auto *ii : m_allImages) {
        if (ii->hash() == wi.m_hash) {
            if (!img.isNull()) {
                ii->setImage(wi, std::move(img));
            }
            if (!thumb.isNull()) {
                ii->setThumb(wi, std::move(thumb), si);
            }
            break;
        }
    }
}

void ImgView::loadedFilenames(QList<WorkItem> is) {
    if (is.isEmpty()) {
        return;
    }

    for (auto i : is) {
        ImageItem *ii = new ImageItem(i.fi);
        m_allImages.push_back(ii);
        connect(ii, &ImageItem::requestImageData, &m_imageloaderqueue, &ImageLoaderQueue::insert);
    }

    // Calculate Position in grid
    int const dim = std::ceil(std::sqrt(m_allImages.size()));
    ImageItem::setXdim(dim);
    int x = 0, y = 0, idx = 0;
    for (auto *ii : m_allImages) {
        ii->grid_idx = QPoint(x, y);
        ii->setIdx(idx++);
        x++;
        if (x >= dim) {
            x = 0;
            y++;
        }
    }

    nextImage(ImgView::FileDir::none);
}

int mapIdxToRange(int idx, int range) {
    if (idx < 0) {
        idx += range;
    } else if (idx >= range) {
        idx -= range;
    }
    return idx;
}

void ImgView::nextImage(FileDir fd) {
    if (m_allImages.isEmpty()) {
        return;
    }
    if (m_mainImage == 0) {
        m_mainImage = m_allImages[0];
    }

    if (fd != FileDir::none) {
        int const nextidx = fitincircularrange((fd == FileDir::next) ? m_mainImage->idx() + 1 : m_mainImage->idx() - 1, m_allImages.size());
        m_mainImage = m_allImages[nextidx];
        autofit();
    } else {
        update();
    }

    // Set Title to new filename
    setTitle();

    // Cache next Images
    if (m_mainImage) {
        int const current_image = m_mainImage->idx();
        int const constexpr images_to_cache = 3;
        for (auto &ii : m_allImages) {
            int dist = std::abs(ii->idx() - current_image);
            dist = std::min(dist, (int) m_allImages.size() - dist);
            ii->preloadNext(dist < images_to_cache);
        }
    }
}

void ImgView::setTitle() {
    if (m_mainImage) {
        ImageItem &is = *m_mainImage;
        emit message(QStringLiteral(u"%1 %2 (%3/%4/%5) (%6x%7 %8kB)")
                         .arg(is.imageinfo().fi.fileName())
                         .arg(is.imageinfo().fi.absoluteFilePath())
                         .arg(is.idx() + 1)
                         .arg(m_thumbcount)
                         .arg(m_allImages.size())
                         .arg(is.size.width())
                         .arg(is.size.height())
                         .arg(is.imageinfo().fi.size() / (1024)));
    }
}

void ImgView::clearImages() {
    m_allImages.clear();
    m_mainImage = nullptr;
    m_thumbcount = 0;
}

void ImgView::closeEvent(QCloseEvent *event) {
    QThreadPool::globalInstance()->waitForDone();
    event->accept();
}

void ImgView::openDatabase() {}

void ImgView::setTransform() {
    m_transform.reset();
    QPoint const center(width() / 2, height() / 2);
    m_transform.translate(m_offset.x() + center.x(), m_offset.y() + center.y());
    m_transform.scale(m_zoom, m_zoom);
    m_transform.translate(-center.x(), -center.y());

    QRectF const logicalRect = m_transform.inverted().mapRect(QRectF(this->rect()));
    for (auto &ii : m_allImages) {
        ii->setVisible(ii->thumbrect().intersects(logicalRect));
    }
}

void ImgView::customContextMenu(QPoint pos) {
    QMenu *menu = new QMenu(this);
    menu->addAction(QStringLiteral(u"Load Image"), [this]() { loadImage(QStringList()); });
    menu->popup(mapToGlobal(pos));
}

void ImgView::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction(); // akzeptiere Drag & Drop
    }
}

void ImgView::dropEvent(QDropEvent *event) {
    QList<QUrl> const droppedUrls = event->mimeData()->urls();
    QStringList filenames;
    for (QUrl const &url : droppedUrls) {
        filenames.push_back(url.toLocalFile());
    }
    if (!filenames.isEmpty()) {
        loadImage(filenames);
    }
}
