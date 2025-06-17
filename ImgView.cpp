#include <QGuiApplication>
#include <QImageReader>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QThreadPool>
#include <QMimeData>

#include "ImgView.h"
#include "IconEngine.h"

void ImgLoaderTask::run(){
    if (imgStruct) {
        QMutexLocker const locker(&imgStruct->mutex);
        QPixmap pix;
        if (pix.load(imgStruct->fi.absoluteFilePath())) {
            imgStruct->img = std::move(pix);
            emit loaded(imgStruct->idx);
        }
    }
}

ImgView::ImgView(QWidget* parent)
{
    setMouseTracking(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setAcceptDrops(true);

    connect(this, &ImgView::customContextMenuRequested, this, &ImgView::customContextMenu);

    resize(800, 600);
    QTimer::singleShot(0, [this]() { resizeEvent(0); });

    QPushButton* btnLoad = new QPushButton("📂", this);
    btnLoad->setToolTip(QStringLiteral(u"Load Image"));
    btnLoad->setFixedSize(24, 24);
    btnLoad->raise();
    m_buttons.push_back(btnLoad);
    connect(btnLoad, &QPushButton::clicked, [this]() {
        loadImage(QStringList());
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

    QSettings settings("Adi", "ImgView");
    m_wheel_zoom = settings.value("Wheel zoom").toBool();
    QPushButton* btnWheelFunction = new QPushButton(m_wheel_zoom ? QStringLiteral(u"🔍") : QStringLiteral(u"🔃"), this);
    btnWheelFunction->setToolTip(QStringLiteral(u"Wheel function"));
    btnWheelFunction->setFixedSize(24, 24);
    btnWheelFunction->raise();
    m_buttons.push_back(btnWheelFunction);
    connect(btnWheelFunction, &QPushButton::clicked, [=,this]() {
        m_wheel_zoom = !m_wheel_zoom;
        btnWheelFunction->setText(m_wheel_zoom ? QStringLiteral(u"🔍") : QStringLiteral(u"🔃"));
        QSettings settings("Adi", "ImgView");
        settings.setValue("Wheel zoom", m_wheel_zoom);
        });

    QByteArrayList const bal = QImageReader::supportedImageFormats();
    m_supported_extensions.clear();
    for (auto const& b : bal) {
        m_supported_extensions.insert(b.toLower());
    }
};

ImgView::~ImgView() {
};

void ImgView::paintEvent(QPaintEvent*){
    QElapsedTimer timer;
    timer.start();
    QPainter p(this);

    if (m_allImages.isEmpty()) {
        //just draw logo
        IconEngine ie;
        QSize const si(160, 160);
        QRect const rect(QPoint(width() / 2 - si.width() / 2, height() / 2 - si.height() / 2), si);
        ie.paint(&p, rect, QIcon::Mode::Normal, QIcon::State::On);
        return;
    }

    if (m_allImages.front()->img.isNull()) {
        // Has to be loaded by loadertreads, now just quit;
    }
    QMutexLocker locker(&m_allImages.front()->mutex);
    QPixmap const& img = m_allImages.front()->img;

    p.setRenderHint(QPainter::RenderHint::TextAntialiasing);

    qreal const dpr = p.device()->devicePixelRatioF();
    p.save();
    p.translate(m_offset);
    qreal wi = width() / 2.;
    qreal he = height() / 2.;
    p.translate(wi, he);
    p.scale(m_zoom, m_zoom);
    p.translate(-wi, -he);

    QSize const scaledSize = img.size().scaled(size(), Qt::KeepAspectRatio);
    QPointF const topLeft(
        (width() - scaledSize.width()) / 2.0,
        (height() - scaledSize.height()) / 2.0);
    QRectF const targetRect(topLeft, scaledSize);
    p.drawPixmap(targetRect, img, QRectF(QPointF(0, 0), img.size()));

    double scx = double(scaledSize.width()) / double(img.width());
    double scy = double(scaledSize.height()) / double(img.height());

    p.scale(scx, scy);

    p.restore();
}

void ImgView::mouseDoubleClickEvent(QMouseEvent*)
{
    autofit();
}

void ImgView::autofit()
{
    m_zoom = 1.;
    m_offset = { 0, 0 };
    update();
}

void ImgView::mousePressEvent(QMouseEvent* event)
{
    m_lastMousePos = event->pos();
    event->accept();
}

void ImgView::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_offset += delta;
        m_lastMousePos = event->pos();
        update(); // trigger repaint
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
        if (ctrl xor !m_wheel_zoom) {
            FileDir const fd = angle > 0 ? FileDir::previous : FileDir::next;
            nextImage(fd);
        } else {
            double const mul = 1. + (0.1 * angle / 120.);
            QPointF const mousePos = event->position();
            QPointF const beforeScale = (mousePos - m_offset - QPointF(width() / 2., height() / 2.)) / m_zoom;
            m_zoom *= mul;
            m_offset = mousePos - (beforeScale * m_zoom) - QPointF(width() / 2., height() / 2.);
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

    if (event) {
        QWidget::resizeEvent(event);
    }
}

void ImgView::keyPressEvent(QKeyEvent* event){
    if (event->key() == Qt::Key_PageDown || event->key() == Qt::Key_Right) {
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
    QSettings settings("Adi", "ImgView");
    QString selectedImage;

    //If we get no list of filenames, ask user
    if (filenames.isEmpty()) {
        QString lastDir = settings.value("LastDirectory", "").toString();
        QString supported(QStringLiteral(u"Image Files ("));
        for (auto const& s : m_supported_extensions) {
            supported += "*." + s + ' ';
        }
        supported += ')';
        selectedImage = QFileDialog::getOpenFileName(this, tr("Load Image File"), lastDir, supported);
        if (!m_supported_extensions.contains(QFileInfo(selectedImage).suffix().toLower())) {
            emit message(QStringLiteral(u"Not supported '%1'").arg(selectedImage));
            return;
        } 
        filenames.push_back(selectedImage);
    }

    if (filenames.size() == 1){
        selectedImage = filenames.at(0);
        filenames = QDir(QFileInfo(filenames.front()).absolutePath()).entryList(QDir::Files | QDir::NoSymLinks | QDir::Readable);
        for (auto& f : filenames){
            f = QFileInfo(selectedImage).absolutePath() + '/' + f;
        }
    }


    //If he have filenames here, delete the internal data and create a new one
    if (!filenames.isEmpty()) {
        for (ImgStruct* img : m_allImages) {
            if (img) {
                img->mutex.lock();
                delete img;
            }
        }
        m_allImages.clear();

        int idx = 0;
        for (auto const& filename : filenames) {
            QFileInfo const fileInfo(filename);
            if (m_supported_extensions.contains(fileInfo.suffix().toLower())) {
                settings.setValue("LastDirectory", fileInfo.absolutePath());
                if (m_supported_extensions.contains(fileInfo.suffix().toLower())) {
                    m_allImages.push_back(new ImgStruct);
                    m_allImages.back()->fn = filename;
                    m_allImages.back()->fi = fileInfo;
                    m_allImages.back()->idx = idx++;
                }
            }
        }

        //Rotate that way, the selected image is the first one
        if (!m_allImages.isEmpty() && !selectedImage.isEmpty()) {
            QFileInfo const fileInfo(selectedImage);
            while (m_allImages.front()->fi.absoluteFilePath().toLower() != fileInfo.absoluteFilePath().toLower()) {
                m_allImages.append(m_allImages.takeFirst());
            }
        }
        nextImage(ImgView::FileDir::none);
    }
}

void ImgView::loaded(size_t idx){
    if (m_allImages.size() > idx){
        if (m_allImages.first()->idx == idx){
            setTitle();
            update();
        }
    }
}

void ImgView::nextImage(FileDir fd)
{
    if (m_allImages.isEmpty()) {
        return;
    }

    // Rotate list once
    if (fd == FileDir::next) {
        m_allImages.append(m_allImages.takeFirst());
    } else if (fd == FileDir::previous) {
        m_allImages.prepend(m_allImages.takeLast());
    }

    //If Image is loaded, set Titlebar
    if (m_allImages.front()->mutex.tryLock()){
        if (!m_allImages.front()->img.isNull()){
            setTitle();
        }
        m_allImages.front()->mutex.unlock();
    }

    // Cache next Images
    for (int i_ = -10; i_ <= 10; i_++) {
        int i = i_;
        if (i < 0){
            i = m_allImages.size() + i;
        }
        if ((m_allImages.size() > i) && (i >= 0)) {
            ImgStruct* is = m_allImages[i];
            if (is->mutex.tryLock()) {
                if (is->img.isNull()){
                    is->visible = i == 0;
                    ImgLoaderTask* ilt = new ImgLoaderTask(is);
                    connect(ilt, &ImgLoaderTask::loaded, this, &ImgView::loaded);
                    QThreadPool::globalInstance()->start(ilt);
                }
                is->mutex.unlock();
            }
        }
    }

    autofit();
}

void ImgView::setTitle(){
    ImgStruct const& is = *m_allImages.first();
    emit message(QStringLiteral(u"%1 (%2/%3) (%4x%5 %6kB)")
            .arg(is.fi.absoluteFilePath())
            .arg(is.idx + 1)
            .arg(m_allImages.size())
            .arg(is.img.width())
            .arg(is.img.height())
            .arg(is.fi.size() / (1024)));
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