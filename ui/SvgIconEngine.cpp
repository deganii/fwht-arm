//
// Created by Ismail Degani on 7/5/2021.
//

#include "SvgIconEngine.h"
#include <QPainter>

SVGIconEngine::SVGIconEngine(const std::string &iconBuffer) {
    data = QByteArray::fromStdString(iconBuffer);
}

void SVGIconEngine::paint(QPainter *painter, const QRect &rect,
                          QIcon::Mode mode, QIcon::State state) {
    QSvgRenderer renderer(data);
    renderer.render(painter, rect);
}

QIconEngine *SVGIconEngine::clone() const { return new SVGIconEngine(*this); }

QPixmap SVGIconEngine::pixmap(const QSize &size, QIcon::Mode mode,
                              QIcon::State state) {
    // This function is necessary to create an EMPTY pixmap. It's called always
    // before paint()
//    QSize quad(size.width()*4, size.height()*4);
    QImage img(size, QImage::Format_ARGB32);
    img.fill(qRgba(0, 0, 0, 0));
    QPixmap pix = QPixmap::fromImage(img, Qt::NoFormatConversion);
    {
        QPainter painter(&pix);
        QRect r(QPoint(0.0, 0.0), size);
        this->paint(&painter, r, mode, state);
    }
    return pix;
}

//QList<QSize> SVGIconEngine::availableSizes(QIcon::Mode mode, QIcon::State state) const {
//    return QList<QSize>();
//    return
//}
