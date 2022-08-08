//
// Created by Ismail Degani on 7/5/2021.
//

#ifndef SVGICONENGINE_H
#define SVGICONENGINE_H

#include <QIconEngine>
#include <QSvgRenderer>

class SVGIconEngine : public QIconEngine {

    QByteArray data;

public:
    explicit SVGIconEngine(const std::string &iconBuffer);
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode,
               QIcon::State state) override;
    QIconEngine *clone() const override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode,
                   QIcon::State state) override;
//    QList<QSize> availableSizes(QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off)  const override;
signals:

public slots:
};

#endif // SVGICONENGINE_H