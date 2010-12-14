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

const int R = 4;

namespace {

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

struct RandomOffsetGenerator
{
    RandomOffsetGenerator(const QImage& realMap, int r):
        width_(realMap.width()),
        height_(realMap.height()),
        bitmap_(realMap) {
    }

    QPoint operator()(QPoint p) {
        int rand_x, rand_y;

        do {
            rand_x = qrand()%width_;
            rand_y = qrand()%height_;
        } while (bitmap_.pixelIndex(rand_x, rand_y));

        return QPoint(rand_x, rand_y) - p;
    }

    QPoint operator()(int x, int y) {
        return operator()(QPoint(x, y));
    }

private:
    int width_;
    int height_;
    QImage bitmap_;
};

QPoint rgb_to_point(QRgb data) {
    qint16* tmp = reinterpret_cast<qint16*>(&data);
    return QPoint(*tmp, *(tmp+1));
}

QRgb point_to_rgb(QPoint point) {
    QRgb result;
    *(reinterpret_cast<qint16*>(&result)) = point.x();
    *(reinterpret_cast<qint16*>(&result) + 1) = point.y();
    return result;
}

}

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
    for (int pass=0; pass<R; ++pass)
        realMap_ = grow_selection(realMap_);

    inputTexture_ = &inputTexture;
    outputMap_ = &outputMap;
    outputTexture_ = inputTexture;

    if (inputTexture.size() != outputMap.size())
        qDebug() << "dimension mismatch";

    RandomOffsetGenerator randoff(realMap_, R);

    QPolygon points_to_fill;
    for (int j=0; j<inputTexture.height(); ++j) {
        for (int i=0, i_end = inputTexture.width(); i<i_end; ++i)
            if (outputMap.pixel(i, j))
                points_to_fill << QPoint(i, j);
    }

    qDebug() << points_to_fill.size() << "to go";

    // this QImage is matrix of offsets
    // 32bit QRgb is interpreted as 2d offset vector of qint16
    offsetMap_ = QImage(inputTexture.size(), QImage::Format_ARGB32);

    // initialize with random offsets for unknown regions
    qDebug() << "generating random offsets";
    offsetMap_.fill(0); // 0 is interpreted as (0, 0) offset
    for (int j=0; j<inputTexture.height(); ++j) {
        const QRgb* outMapPixel = reinterpret_cast<const QRgb*>(outputMap.scanLine(j));
        qint16* offMapPixel = reinterpret_cast<qint16*>(offsetMap_.scanLine(j));

        for (int i=0, i_end = inputTexture.width(); i<i_end; ++i) {
            if (*outMapPixel) {
                QPoint random_offset = randoff(i, j);
                *offMapPixel = random_offset.x();
                *(offMapPixel+1) = random_offset.y();
            }
            ++outMapPixel;
            offMapPixel += 2;
        }
    }

    scoreMap_ = QImage(inputTexture.size(), QImage::Format_ARGB32);
    scoreMap_.fill(0);

    // fill result and scoremap using offsets
    for (int j=0; j<inputTexture.height(); ++j)
        for (int i=0, i_end = inputTexture.width(); i<i_end; ++i) {
            outputTexture_.setPixel(i, j,
                    inputTexture.pixel(QPoint(i, j) + rgb_to_point(offsetMap_.pixel(i, j))));
            if (outputMap_->pixel(i, j))
                scoreMap_.setPixel(i, j, (QRgb)INT_MAX);
        }

    QPolygon some_neighbours_even, some_neighbours_odd;
    some_neighbours_even << QPoint(0, -1) << QPoint(-1, 0);
    some_neighbours_odd << QPoint(0, 1) << QPoint(1, 0);

    for (int pass=0; pass<60; ++pass) {
        // refine pass
        // there are three kinds of places where we can look for better matches:
        // 1. Obviously, random places
        // 2. Propagation: trying points near our neighbour's source
        // 3. points near our source
        std::cout << "." << std::flush;

        const QPolygon& some_neighbours = (pass%2)?some_neighbours_even:some_neighbours_odd;

        foreach(QPoint p, points_to_fill) {
            QPoint best_offset = rgb_to_point(offsetMap_.pixel(p));

            // since the last iteration scoreMap_ became obsoleted
            // int best_score = (int)scoreMap_.pixel(p);
            int best_score = INT_MAX;
            updateSource(p, &best_offset, best_offset, &best_score);

            // random search
            for (int attempt=0; attempt<10; ++attempt)
                updateSource(p, &best_offset, randoff(p), &best_score);

            foreach (QPoint neighbour_minus_p, some_neighbours) {
                if (outputMap.pixel(p + neighbour_minus_p)) {
                    // our neighbour is unknown point too
                    // maybe his offset is better than ours
                    QPoint neighbours_offset = rgb_to_point(offsetMap_.pixel(p+neighbour_minus_p));
                    updateSource(p, &best_offset, neighbours_offset, &best_score);
                }
            }

            // save found offset
            offsetMap_.setPixel(p, point_to_rgb(best_offset));

            // transfer pixel
            outputTexture_.setPixel(p,
                    inputTexture.pixel(p + rgb_to_point(offsetMap_.pixel(p))));
        }
        std::reverse(points_to_fill.begin(), points_to_fill.end());
    }
    std::cout << std::endl;

    return outputTexture_;
}

bool Resynthesizer::updateSource(QPoint p, QPoint* best_offset,
    QPoint candidate_offset, int* best_score)
{
    QRect bounds(QPoint(0, 0), inputTexture_->size());

    // source point
    QPoint s = p + candidate_offset;

    // fuck the edge cases
    if (p.x() < R || p.x() >= inputTexture_->width()-R || p.y() < R || p.y() >= inputTexture_->height()-R ||
        s.x() < R || s.x() >= inputTexture_->width()-R || s.y() < R || s.y() >= inputTexture_->height()-R)
        return false;

    // we need only true real patches as sources
    if (!realMap_.pixelIndex(s))
        return false;

    if (!bounds.contains(p) || !bounds.contains(s))
        return false;

    int score = 0;
    for (int j=-R; j<=R; ++j) {
        for (int i=-R; i<=R; ++i) {
            QPoint area_offset = QPoint(i, j);
            QPoint near_p = p + area_offset;

            if (outputMap_->pixel(near_p)) {
                // *near_p* has his opinion on what color *p* should be 
                // we need to check how that fits with s
                QPoint his_offset = rgb_to_point(offsetMap_.pixel(near_p));

                // TODO: weight opinions based on scoreMap_
                QPoint his_opinion = p + his_offset;

                if (!bounds.contains(his_opinion)) {
                    score += 255*255*4;
                } else {
                    QRgb s_pixel = outputTexture_.pixel(s);
                    QRgb o_pixel = outputTexture_.pixel(his_opinion);

                    quint8* s_rgb = reinterpret_cast<quint8*>(&s_pixel);
                    quint8* o_rgb = reinterpret_cast<quint8*>(&o_pixel);

                    score += ssd4(s_rgb, o_rgb);
                }
            } else {
                QPoint near_s = s + area_offset;

                QRgb np_pixel = outputTexture_.pixel(near_p);
                QRgb ns_pixel = outputTexture_.pixel(near_s);

                quint8* np_rgb = reinterpret_cast<quint8*>(&np_pixel);
                quint8* ns_rgb = reinterpret_cast<quint8*>(&ns_pixel);

                score += ssd4(np_rgb, ns_rgb);
            }
        }
    }

    if (*best_score <= score)
        return false;

    *best_score = score;
    scoreMap_.setPixel(p, (QRgb)score);
    *best_offset = candidate_offset;
    return true;
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

            int value = 255;
            int saturation = 255*sqrt(qreal(o.x()*o.x() + o.y()*o.y())/
                    (offsetMap_.width()*offsetMap_.width() + offsetMap_.height()*offsetMap_.height()));
            QColor c = QColor::fromHsv(hue, saturation, value);
            result.setPixel(i, j, c.rgb());
        }

    return result;
}

QImage Resynthesizer::scoreMap()
{
    // TODO: convert to something nice
    return scoreMap_.convertToFormat(QImage::Format_RGB32);
}

