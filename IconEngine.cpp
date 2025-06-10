#include "IconEngine.h"
#include <QPainter>

void IconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode /*mode*/, QIcon::State /*state*/)
{

    qreal const inv_dpr = 1. / painter->device()->devicePixelRatioF();

    QString const str1(QStringLiteral(u"👁️‍🗨️"));
    QString const str2(QStringLiteral(u"👁️‍🗨️"));
    QString const str(QStringLiteral(u"👁️‍🗨️"));

    qreal const wi = rect.width();
    qreal const he = rect.height();
    qreal const cl = 0.0 * wi;
    qreal const clx = cl + rect.left();
    qreal const cly = cl + rect.top();

    painter->fillRect(rect, Qt::transparent);
    painter->setRenderHint(QPainter::RenderHint::Antialiasing);
    painter->setRenderHint(QPainter::RenderHint::TextAntialiasing);
    painter->setPen(m_col);
    painter->setBrush(m_col);

    // Calculate font size
    QFont font = painter->font();
    font.setFamily(QStringLiteral(u"Times New Roman"));
    font.setBold(true);
    font.setPointSizeF(100);
    QFontMetrics fm(font);
    qreal len;
    if (wi * inv_dpr <= 22.) {
        len = fm.horizontalAdvance(str);
    } else {
        len = std::max(fm.horizontalAdvance(str1), fm.horizontalAdvance(str2));
    }
    qreal const pointsi = (wi - 2. * cl) / len * 100.;
    font.setPointSizeF(pointsi);
    painter->setFont(font);

    // Draw Text to Pixmap
    if (wi * inv_dpr <= 22) {
        painter->drawText(QRectF(clx, cly, wi - 2. * cl, he - 2 * cl), Qt::AlignCenter | Qt::TextDontClip | Qt::TextSingleLine, str);
    } else {
        painter->drawText(QRectF(clx, cly, wi - 2. * cl, he * 0.5 - 2 * cl), Qt::AlignCenter | Qt::TextDontClip | Qt::TextSingleLine, str1);
        painter->drawText(QRectF(clx, cly + he * 0.4, wi - 2. * cl, he * 0.6 - 2 * cl), Qt::AlignCenter | Qt::TextDontClip | Qt::TextSingleLine, str2);
    }
}

QPixmap IconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state)
{
    QImage img(size, QImage::Format_ARGB32);
    img.fill(qRgba(0, 0, 0, 0));
    QPixmap pix = QPixmap::fromImage(img, Qt::NoFormatConversion);
    QPainter painter(&pix);
    paint(&painter, QRect(0, 0, size.width(), size.height()), mode, state);
    return pix;
}
