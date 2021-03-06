#include "resynthesizer.h"

#include <QColor>
#include <QDebug>
#include <QVector>
#include <QtGlobal>
#include <algorithm>
#include <iostream>
#include <qmath.h>

#include "pixel.h"
#include "randomoffsetgenerator.h"
#include "similaritymapper.h"
#include "utils.h"

const int R = 4;
const int PASS_COUNT = 50;
const int LOD_MAX = 3;

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


QImage Resynthesizer::inpaintHier(const QImage& inputTexture,
                              const QImage& outputMap)
{
    bool first_pass = true;
    COWMatrix<QPoint> lodOffsetMap;

    for (int lod_level=LOD_MAX; lod_level>=0; --lod_level) {
        QSize lodSize = inputTexture.size()/(1<<lod_level);

        if (!first_pass)
            lodOffsetMap = resize_offset_map(lodOffsetMap, lodSize);

        QImage lodInputTexture = inputTexture.scaled(lodSize,
                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        QImage lodOutputMap = outputMap.scaled(lodSize,
                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        lodOffsetMap = buildOffsetMap(lodInputTexture, lodOutputMap,
               first_pass?COWMatrix<QPoint>():lodOffsetMap);

        if (first_pass)
            first_pass = false;

        outputTexture_.save(QString("tmp/lod_%1.png").arg(lod_level));
    }
    return outputTexture_;
}

COWMatrix<QPoint> Resynthesizer::buildOffsetMap(const QImage& inputTexture,
                      const QImage& outputMap,
                      const COWMatrix<QPoint>& hint)
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

    if (hint.isNull()) {
        offsetMap_ = COWMatrix<QPoint>(inputTexture.size());
        // generate initial offsetmap
        offsetMap_.fill(QPoint(0, 0));

        RandomOffsetGenerator rog(realMap_, R);
        for (int j=0; j<outputMap.height(); ++j)
            for (int i=0; i<outputMap.width(); ++i)
                if (!realMap_.pixelIndex(i, j))
                    offsetMap_.set(i, j, rog(i, j));
    } else {
        offsetMap_ = hint;
    }

    // fill offsetmap with random offsets for unknows points
    confidenceMap_ = QVector<double>(realMap_.width()*realMap_.height(), 1.0);
    for (int j=0; j<outputMap.height(); ++j)
        for (int i=0; i<outputMap.width(); ++i)
            if (!realMap_.pixelIndex(i, j))
                confidenceMap_[j*outputMap.width()+i] = 1e-10;

    mergePatches(false);

    sm.init(inputTexture, outputTexture_, realMap_, realMap_);

    double prev_mean_score = 4.f*256*256;
    int prev_max_score = INT_MAX;
    for (int pass=0; pass<PASS_COUNT; ++pass) {
        // update offsetMap_
        offsetMap_ = sm.iterate(outputTexture_);

        reliabilityMap_ = sm.reliabilityMap();
        mergePatches(true);

        double mean_score = sm.meanScore();
        int max_score = sm.maxScore();
        if (mean_score > prev_mean_score*0.995 && mean_score <= prev_mean_score &&
            max_score > prev_max_score*0.995 && max_score <= prev_max_score) {
            // local minimum sort of found
            break;
        }
        prev_mean_score = mean_score;
        prev_max_score = max_score;
    }

    return offsetMap_;
}

void Resynthesizer::mergePatches(bool weighted)
{
    QRect bounds = outputTexture_.rect();
    int width  = offsetMap_.width();
    int height = offsetMap_.height();

    QVector<qreal> new_confidence_map(confidenceMap_);

    for (int j=0; j<height; ++j)
        for (int i=0; i<width; ++i)
            if (!realMap_.pixelIndex(i, j)) {
                QPoint p(i, j);
                qreal r = 0.0, g = 0.0, b = 0.0;
                qreal new_confidence = 0.0;
                qreal confidence_sum = 0.0;
                qreal weight_sum = 0.0;

                for (int dj=-R; dj<=R; ++dj)
                    for (int di=-R; di<=R; ++di) {
                        QPoint near_p = p + QPoint(di, dj);
                        if (!bounds.contains(near_p))
                            continue;

                        QPoint opinion_point = p + offsetMap_.get(near_p);

                        QColor c(inputTexture_->pixel(opinion_point));

                        int idx = (near_p).y()*width + (near_p).x();
                        qreal weight = (weighted)
                                            ?(reliabilityMap_.get(near_p)*confidenceMap_[idx])
                                            :1.0;

                        new_confidence += confidenceMap_[idx]*weight;

                        r += c.red()*weight;
                        g += c.green()*weight;;
                        b += c.blue()*weight;;

                        weight_sum += weight;
                    }

                if (0.0 == weight_sum) {
                    qDebug("this is bad");
                    continue;
                }

                r /= weight_sum;
                g /= weight_sum;
                b /= weight_sum;

                outputTexture_.setPixel(p, QColor(r, g, b).rgb());
                new_confidence_map[p.y()*width + p.x()] = new_confidence/weight_sum;
            }
    confidenceMap_ = new_confidence_map;
}

