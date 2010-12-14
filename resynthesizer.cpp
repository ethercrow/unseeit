#include "resynthesizer.h"

#include <QtGlobal>
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

    for (int pass=0; pass<20; ++pass) {
        // refine pass
        // there are three kinds of places where we can look for better matches:
        // 1. Obviously, random places
        // 2. Propagation: trying points near our neighbour's source
        // 3. points near our source
        qDebug() << "refinement pass";

        const QPolygon& some_neighbours = (pass%2)?some_neighbours_even:some_neighbours_odd;

        foreach(QPoint p, points_to_fill) {
            QPoint best_offset = rgb_to_point(offsetMap_.pixel(p));

            // since the last iteration scoreMap_ became obsoleted
            // int best_score = (int)scoreMap_.pixel(p);
            int best_score = INT_MAX;
            updateSource(p, &best_offset, best_offset, &best_score);

            // // random search
            // for (int attempt=0; attempt<30; ++attempt)
            //     updateSource(p, &best_offset, randoff(p), &best_score);

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

    return outputTexture_;
}

bool Resynthesizer::updateSource(QPoint p, QPoint* best_offset,
    QPoint candidate_offset, int* best_score)
{
    int R = 4;

    QRect bounds(QPoint(0, 0), inputTexture_->size());

    // source point
    QPoint s = p + candidate_offset;

    // fuck the bounds
    if (p.x() < R || p.x() >= inputTexture_->width()-R || p.y() < R || p.y() >= inputTexture_->height()-R ||
        s.x() < R || s.x() >= inputTexture_->width()-R || s.y() < R || s.y() >= inputTexture_->height()-R)
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

                QPoint his_opinion = p + his_offset;

                if (!bounds.contains(his_opinion)) {
                    score += 255*255*4;
                } else {
                    QRgb s_pixel = outputTexture_.pixel(s);
                    QRgb o_pixel = outputTexture_.pixel(his_opinion);

                    quint8* s_rgb = reinterpret_cast<quint8*>(&s_pixel);
                    quint8* o_rgb = reinterpret_cast<quint8*>(&o_pixel);

                    for (int c=0; c<4; ++c) {
                        int d = ((int)*s_rgb-(int)*o_rgb);
                        score += d*d/(1+i*i+j*j);
                        ++s_rgb;
                        ++o_rgb;
                    }
                }
            }

            QPoint near_s = s + area_offset;

            QRgb np_pixel = outputTexture_.pixel(near_p);
            QRgb ns_pixel = outputTexture_.pixel(near_s);

            quint8* np_rgb = reinterpret_cast<quint8*>(&np_pixel);
            quint8* ns_rgb = reinterpret_cast<quint8*>(&ns_pixel);

            for (int c=0; c<4; ++c) {
                int d = ((int)*np_rgb-(int)*ns_rgb);
                d += d*3*(!outputMap_->pixel(near_p));
                score += d*d;
                ++np_rgb;
                ++ns_rgb;
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
    // TODO: convert to something nice
    return offsetMap_;
}

QImage Resynthesizer::scoreMap()
{
    // TODO: convert to something nice
    return scoreMap_.convertToFormat(QImage::Format_RGB32);
}

int Resynthesizer::coherence(QPoint p1, QPoint p2, int R)
{
    int result;
    QRect bounds(QPoint(0, 0), inputTexture_->size());
    QPoint d = p2 - p1;

    QPoint s1 = p1 + rgb_to_point(offsetMap_.pixel(p1));
    QPoint s2 = p2 + rgb_to_point(offsetMap_.pixel(p2));

    for (int j=qMax(-R, -R+d.y()); j<=qMin(R, R+d.y()); ++j)
        for (int i=qMax(-R, -R+d.x()); i<=qMin(R, R+d.x()); ++i) {
            QPoint near_offset = QPoint(i, j);
            QPoint near_s1 = s1 + near_offset;
            QPoint near_s2 = s2 - d + near_offset;

            if (bounds.contains(near_s1) && bounds.contains(near_s2)) {

                QRgb s1_pixel = outputTexture_.pixel(near_s1);
                QRgb s2_pixel = outputTexture_.pixel(near_s2);

                quint8* s1_rgb = reinterpret_cast<quint8*>(&s1_pixel);
                quint8* s2_rgb = reinterpret_cast<quint8*>(&s2_pixel);

                for (int c=0; c<4; ++c) {
                    int d = ((int)*s1_rgb-(int)*s2_rgb);
                    result += d*d;
                    ++s1_rgb;
                    ++s2_rgb;
                }
            } else {
                result += 256*256*3;
            }
        }

    return 0;
}
