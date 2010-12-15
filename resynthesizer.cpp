#include "resynthesizer.h"

#include <iostream>
#include <QColor>
#include <qmath.h>
#include <QtGlobal>
#include "utils.h"
#include <QDebug>
#include <QtGlobal>
#include <algorithm>
#include "pixel.h"
#include "similaritymapper.h"
#include "randomoffsetgenerator.h"

const int R = 4;

namespace {

// propagate 0s in mono QImage
QImage grow_selection(QImage arg)
{
    QRect bounds(QPoint(0, 0), arg.size());
    QImage result = arg;
    QPolygon offsets;
    offsets << QPoint(0, 1) << QPoint(1, 0) << QPoint(-1, 0) << QPoint(0, -1);
    for (int j=0; j<arg.height(); ++j)
        for (int i=0; i<arg.width(); ++i) {
            QPoint p(i, j);
            foreach(QPoint o, offsets)
                if (bounds.contains(p+o) && !arg.pixelIndex(p+o))
                    result.setPixel(p, 0);
        }
    return result;
}

};

QImage Resynthesizer::inpaint(const QImage& inputTexture,
                              const QImage& outputMap)
{
    TRACE_ME

    realMap_ = QImage(outputMap.width(), outputMap.height(), QImage::Format_Mono);
    realMap_.fill(1);
    for (int j=0; j<outputMap.height(); ++j)
        for (int i=0; i<outputMap.width(); ++i)
            if (outputMap.pixel(i, j))
                realMap_.setPixel(i, j, 0);
    for (int pass=0; pass<=R; ++pass)
        realMap_ = grow_selection(realMap_);

    SimilarityMapper sm;

    inputTexture_ = &inputTexture;
    outputTexture_ = inputTexture;

    offsetMap_ = inputTexture;
    // generate initial offsetmap
    offsetMap_.fill(0);

    // fill offsetmap with random offsets for unknows points
    RandomOffsetGenerator rog(realMap_, R);
    for (int j=0; j<outputMap.height(); ++j)
        for (int i=0; i<outputMap.width(); ++i)
            if (!realMap_.pixelIndex(i, j))
                offsetMap_.setPixel(i, j, point_to_rgb(rog(i, j)));

    mergePatches();

    for (int pass=0; pass<50; ++pass) {
        // update offsetMap_
        offsetMap_ = sm.calculateVectorMap(inputTexture, outputTexture_, realMap_);

        mergePatches();
    }

    return outputTexture_;
}

void Resynthesizer::mergePatches()
{
    // TODO: weighted mean

    QRect bounds(QPoint(0, 0), outputTexture_.size());

    for (int j=0; j<outputTexture_.height(); ++j)
        for (int i=0; i<outputTexture_.width(); ++i)
            if (!realMap_.pixelIndex(i, j)) {
                QPoint p(i, j);
                int r = 0, g = 0, b = 0;

                for (int dj=-R; dj<=R; ++dj)
                    for (int di=-R; di<=R; ++di) {
                        QPoint dp = QPoint(di, dj);
                        QPoint opinion_point = p + rgb_to_point(offsetMap_.pixel(p+dp));

                        if (!bounds.contains(opinion_point))
                            qDebug() << "WTF1?!";

                        if (!realMap_.pixelIndex(opinion_point))
                            qDebug() << "WTF2?!";

                        QColor c(inputTexture_->pixel(opinion_point));
                        r += c.red();
                        g += c.green();
                        b += c.blue();
                    }

                r /= (2*R+1)*(2*R+1);
                g /= (2*R+1)*(2*R+1);
                b /= (2*R+1)*(2*R+1);

                outputTexture_.setPixel(p, QColor(r, g, b).rgb());
            }
}

QImage Resynthesizer::offsetMap()
{
    QImage result = offsetMap_.convertToFormat(QImage::Format_RGB32);

    // angle -> hue
    // magnitude -> saturation
    for (int j=0; j<offsetMap_.height(); ++j)
        for (int i=0; i<offsetMap_.width(); ++i) {
            QPoint o = rgb_to_point(offsetMap_.pixel(i, j));
            int hue = 180/M_PI*qAtan2(o.x(), o.y());
            if (hue<0)
                hue += 360;
            if (hue==360)
                hue = 0;

            int value = 128;
            int saturation = 255*sqrt(qreal(o.x()*o.x() + o.y()*o.y())/
                    (offsetMap_.width()*offsetMap_.width() + offsetMap_.height()*offsetMap_.height()));
            QColor c = QColor::fromHsv(hue, saturation, value);
            result.setPixel(i, j, c.rgb());
        }

    return result;
}

QImage Resynthesizer::realMap()
{
    return realMap_;
}
