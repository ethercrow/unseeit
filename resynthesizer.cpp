#include "resynthesizer.h"

#include <QColor>
#include <QDebug>
#include <QVector>
#include <QtGlobal>
#include <QtGlobal>
#include <algorithm>
#include <iostream>
#include <qmath.h>

#include "pixel.h"
#include "randomoffsetgenerator.h"
#include "similaritymapper.h"
#include "utils.h"

const int R = 4;
const double SIGMA2 = 100.f;
const int PASS_COUNT = 30;

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

    realMap_ = QImage(outputMap.size(), QImage::Format_Mono);
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
    confidenceMap_ = QVector<double>(realMap_.width()*realMap_.height(), 1.0);
    for (int j=0; j<outputMap.height(); ++j)
        for (int i=0; i<outputMap.width(); ++i)
            if (!realMap_.pixelIndex(i, j)) {
                offsetMap_.setPixel(i, j, point_to_rgb(rog(i, j)));
                confidenceMap_[j*outputMap.width()+i] = 1e-10;
            }

    mergePatches(false);

    sm.init(inputTexture, outputTexture_, realMap_);

    for (int pass=0; pass<PASS_COUNT; ++pass) {
        std::cout << "." << std::flush;
        // update offsetMap_
        offsetMap_ = sm.iterate(outputTexture_);
        scoreMap_ = sm.scoreMap();

        mergePatches(true);
    }
    std::cout << std::endl;

    return outputTexture_;
}

void Resynthesizer::mergePatches(bool weighted)
{
    QRect bounds(QPoint(0, 0), outputTexture_.size());
    int width = offsetMap_.width();

    QVector<qreal> weightMap(offsetMap_.height()*width, 1.0);
    QVector<qreal> new_confidence_map(confidenceMap_);

    if (weighted)
        for (int j=0; j<offsetMap_.height(); ++j)
            for (int i=0; i<width; ++i) {
                QPoint p(i, j);
                if (!realMap_.pixelIndex(i, j)) {
                    int score = (int)scoreMap_->pixel(p);
                    qreal reliability = qExp(-score/SIGMA2);
                    weightMap[j*width+i] = reliability;
                }
            }

    for (int j=0; j<outputTexture_.height(); ++j)
        for (int i=0; i<width; ++i)
            if (!realMap_.pixelIndex(i, j)) {
                QPoint p(i, j);
                qreal r = 0.0, g = 0.0, b = 0.0;
                qreal new_confidence = 0.0;
                qreal confidence_sum = 0.0;
                qreal weight_sum = 0.0;

                for (int dj=-R; dj<=R; ++dj)
                    for (int di=-R; di<=R; ++di) {
                        QPoint dp = QPoint(di, dj);
                        QPoint opinion_point = p + rgb_to_point(offsetMap_.pixel(p+dp));

                        QColor c(inputTexture_->pixel(opinion_point));

                        int idx = (p+dp).y()*width + (p+dp).x();
                        qreal weight = (weighted)
                                            ?(weightMap[idx]*confidenceMap_[idx])
                                            :1.0;

                        new_confidence += confidenceMap_[idx]*weight;

                        r += c.red()*weight;
                        g += c.green()*weight;;
                        b += c.blue()*weight;;

                        weight_sum += weight;
                    }

                r /= weight_sum;
                g /= weight_sum;
                b /= weight_sum;

                outputTexture_.setPixel(p, QColor(r, g, b).rgb());
                new_confidence_map[p.y()*width + p.x()] = new_confidence/weight_sum;
            }
    confidenceMap_ = new_confidence_map;
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

