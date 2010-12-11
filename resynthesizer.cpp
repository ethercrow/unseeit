#include "resynthesizer.h"

#include "utils.h"
#include <QDebug>
#include <QtGlobal>
#include <algorithm>

namespace {

struct RandomOffsetGenerator
{
    RandomOffsetGenerator(const QImage& outputMap):
        width_(outputMap.width()), height_(outputMap.height()),
        outputMap_(outputMap) {
    }

    QPoint operator()(QPoint p) {
        int rand_x, rand_y;

        do {
            rand_x = qrand()%width_;
            rand_y = qrand()%height_;
        } while (outputMap_.pixel(rand_x, rand_y));

        return QPoint(rand_x, rand_y) - p;
    }

    QPoint operator()(int x, int y) {
        return operator()(QPoint(x, y));
    }

private:
    int width_;
    int height_;
    const QImage& outputMap_;
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

    inputTexture_ = &inputTexture;
    outputMap_ = &outputMap;
    outputTexture_ = inputTexture;

    if (inputTexture.size() != outputMap.size())
        qDebug() << "dimension mismatch";

    RandomOffsetGenerator randoff(outputMap);

    QPolygon points_to_fill;
    for (int j=0; j<inputTexture.height(); ++j) {
        for (int i=0, i_end = inputTexture.width(); i<i_end; ++i)
            if (outputMap.pixel(i, j))
                points_to_fill << QPoint(i, j);
    }

    qDebug() << points_to_fill.size() << "to go";

    // this QImage is matrix of offsets
    // 32bit QRgb is interpreted as 2d offset vector of qint16
    QImage offsetMap(inputTexture.size(), QImage::Format_ARGB32);

    // initialize with random offsets for unknown regions
    qDebug() << "generating random offsets";
    offsetMap.fill(0); // 0 is interpreted as (0, 0) offset
    for (int j=0; j<inputTexture.height(); ++j) {
        const QRgb* outMapPixel = reinterpret_cast<const QRgb*>(outputMap.scanLine(j));
        qint16* offMapPixel = reinterpret_cast<qint16*>(offsetMap.scanLine(j));

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

    QPolygon four_neighbours;
    four_neighbours << QPoint(0, 1) << QPoint(0, -1) << QPoint(1, 0) << QPoint(-1, 0);

    for (int pass=0; pass<20; ++pass) {

        // fill result using offsets
        for (int j=0; j<inputTexture.height(); ++j)
            for (int i=0, i_end = inputTexture.width(); i<i_end; ++i)
                outputTexture_.setPixel(i, j,
                        inputTexture.pixel(QPoint(i, j) + rgb_to_point(offsetMap.pixel(i, j))));

        // refine pass
        // there are three kinds of places where we can look for better matches:
        // 1. Obviously, random places
        // 2. points near our neighbour's source
        // 3. points near our source
        qDebug() << "refinement pass";
        foreach(QPoint p, points_to_fill) {
            QPoint best_offset = rgb_to_point(offsetMap.pixel(p));
            int best_score = INT_MAX;
            updateSource(p, &best_offset, best_offset, &best_score);

            for (int attempt=0; attempt<4; ++attempt)
                updateSource(p, &best_offset, randoff(p), &best_score);

            foreach (QPoint neighbour_minus_p, four_neighbours) {
                updateSource(p, &best_offset, best_offset + neighbour_minus_p, &best_score);
                if (outputMap.pixel(p + neighbour_minus_p)) {
                    // our neighbour is unknown point too
                    QPoint neighbours_offset = rgb_to_point(offsetMap.pixel(p+neighbour_minus_p));
                    updateSource(p, &best_offset, neighbours_offset - neighbour_minus_p, &best_score);
                }
            }

            offsetMap.setPixel(p, point_to_rgb(best_offset));
        }
        std::reverse(points_to_fill.begin(), points_to_fill.end());
    }

    // fill result using offsets
    qDebug() << "generating resulting image";
    for (int j=0; j<inputTexture.height(); ++j)
        for (int i=0, i_end = inputTexture.width(); i<i_end; ++i)
            outputTexture_.setPixel(i, j,
                    inputTexture.pixel(QPoint(i, j) + rgb_to_point(offsetMap.pixel(i, j))));

    return outputTexture_;
}

bool Resynthesizer::updateSource(QPoint p, QPoint* best_offset,
    QPoint candidate_offset, int* best_score)
{
    int R = 7;

    // source point
    QPoint s = p + candidate_offset;

    // fuck the bounds
    if (p.x() < R || p.x() >= inputTexture_->width()-R || p.y() < R || p.y() >= inputTexture_->height()-R ||
        s.x() < R || s.x() >= inputTexture_->width()-R || s.y() < R || s.y() >= inputTexture_->height()-R)
        return false;

    int score = 0;
    for (int j=-R; j<=R; ++j) {
        for (int i=-R; i<=R; ++i) {
            QPoint area_offset = QPoint(i, j);
            QPoint near_p = p + area_offset;
            QPoint near_s = s + area_offset;
            // if (outputMap_->pixel(near_s)) {
            //     score += 255*255*4;
            // } else {
                QRgb p_pixel = outputTexture_.pixel(near_p);
                QRgb s_pixel = inputTexture_->pixel(near_s);

                quint8* p_rgb = reinterpret_cast<quint8*>(&p_pixel);
                quint8* s_rgb = reinterpret_cast<quint8*>(&s_pixel);
                for (int c=0; c<4; ++c) {
                    int d = ((int)*p_rgb-(int)*s_rgb);
                    score += d*d;
                    ++p_rgb;
                    ++s_rgb;
                }
            //}
        }
    }

    if (*best_score <= score)
        return false;

    *best_score = score;
    *best_offset = candidate_offset;
    return true;
}

