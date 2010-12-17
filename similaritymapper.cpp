#include "similaritymapper.h"

#include <algorithm>
#include <iostream>
#include <qmath.h>

#include "pixel.h"
#include "randomoffsetgenerator.h"
#include "utils.h"

const int R = 4;
const int PASS_COUNT = 10;
const double SIGMA2 = 100.f;

void SimilarityMapper::init(const QImage& src,
        const QImage& dst, const QImage& srcMask)
{
    // offsetmap has the same dimensions as dst
    offsetMap_ = dst;
    src_ = &src;
    srcMask_ = &srcMask;

    RandomOffsetGenerator rog(srcMask, R);

    scoreMap_ = dst;
    scoreMap_.fill(0);

    // fill offsetmap with random offsets for unknows points
    offsetMap_.fill(0);
    for (int j=0; j<offsetMap_.height(); ++j)
        for (int i=0; i<offsetMap_.width(); ++i)
            if (!srcMask.pixelIndex(i, j))
                offsetMap_.setPixel(i, j, point_to_rgb(rog(i, j)));

    // create list of unknown points
    for (int j=0; j<dst.height(); ++j)
        for (int i=0, i_end = dst.width(); i<i_end; ++i)
            if (!srcMask.pixelIndex(i, j))
                pointsToFill_ << QPoint(i, j);

    qDebug() << pointsToFill_.size() << "points to fill";
}

QImage SimilarityMapper::iterate(const QImage& dst)
{
    dst_ = &dst;
    int init_search_range = qMax(src_->width(), src_->height());

    QPolygon neighbour_offsets_even, neighbour_offsets_odd;
    neighbour_offsets_even << QPoint(0, -1) << QPoint(-1, 0);
    neighbour_offsets_odd << QPoint(0, 1) << QPoint(1, 0);

    RandomOffsetGenerator rog(*srcMask_, R);

    // // fill offsetmap with random offsets for unknows points
    // offsetMap_.fill(0);
    // for (int j=0; j<offsetMap_.height(); ++j)
    //     for (int i=0; i<offsetMap_.width(); ++i)
    //         if (!srcMask_->pixelIndex(i, j))
    //             offsetMap_.setPixel(i, j, point_to_rgb(rog(i, j)));

    for (int pass=0; pass<PASS_COUNT; ++pass) {
        // refine pass
        // there are two kinds of places where we can look for better matches:
        // 1. Obviously, random places
        // 2. Propagation: trying points near our neighbour's source
        const QPolygon& neighbour_offsets = (pass%2)?neighbour_offsets_odd:neighbour_offsets_even;

        foreach(QPoint p, pointsToFill_) {
            QPoint best_offset = rgb_to_point(offsetMap_.pixel(p));

            int best_score = INT_MAX;
            updateSource(p, &best_offset, best_offset, &best_score);

            if (best_score == 0)
                continue;

            // random search
            for (int range=init_search_range; range>0; range/=2) {
                QPoint o(best_offset);
                o.rx() += qrand()%(2*range) - range;
                o.ry() += qrand()%(2*range) - range;
                updateSource(p, &best_offset, o, &best_score);
            }

            // propagate good guess from left and above (right and below on odd iterations)
            foreach (QPoint dp, neighbour_offsets) {
                if (!srcMask_->pixelIndex(p + dp)) {
                    // our neighbour is unknown point too
                    // maybe his offset is better than ours
                    QPoint neighbours_offset = rgb_to_point(offsetMap_.pixel(p+dp));
                    if (srcMask_->pixelIndex(p + neighbours_offset))
                        updateSource(p, &best_offset, neighbours_offset, &best_score);
                }
            }

            // save found offset
            offsetMap_.setPixel(p, point_to_rgb(best_offset));
        }
        std::reverse(pointsToFill_.begin(), pointsToFill_.end());
    }

    report_max_score();

    return offsetMap_;
}

void SimilarityMapper::report_max_score()
{
    int max_score = 0;
    int min_score = INT_MAX;
    double mean_score = 0;

    foreach(QPoint p, pointsToFill_) {
        int score = (int)scoreMap_.pixel(p);
        max_score = qMax(max_score, score);
        min_score = qMin(min_score, score);
        mean_score += score;
    }

    mean_score /= pointsToFill_.size();

    qDebug() << "score : max =" << max_score
        << "mean =" << mean_score
        << "min =" << min_score;
}

bool SimilarityMapper::updateSource(QPoint p, QPoint* best_offset,
    QPoint candidate_offset, int* best_score)
{
    QRect bounds(QPoint(0, 0), src_->size());

    // source point
    QPoint s = p + candidate_offset;

    // fuck the edge cases
    if (p.x() < R || p.x() >= src_->width()-R || p.y() < R || p.y() >= src_->height()-R ||
        s.x() < R || s.x() >= src_->width()-R || s.y() < R || s.y() >= src_->height()-R)
        return false;

    // we need only true real patches as sources
    if (!srcMask_->pixelIndex(s))
        return false;

    if (!bounds.contains(p) || !bounds.contains(s))
        return false;

    double score = 0;
    double weight_sum = 0;
    for (int j=-R; j<=R; ++j)
        for (int i=-R; i<=R; ++i) {
            QPoint dp = QPoint(i, j);

            QPoint near_p = p + dp;
            QPoint near_s = s + dp;

            QRgb ns_pixel = src_->pixel(near_s);
            QRgb np_pixel = dst_->pixel(near_p);

            quint8* ns_rgb = reinterpret_cast<quint8*>(&ns_pixel);
            quint8* np_rgb = reinterpret_cast<quint8*>(&np_pixel);

            int his_score = (int)scoreMap_.pixel(near_p);
            double weight = qExp(-his_score/SIGMA2);

            score += ssd4(ns_rgb, np_rgb)*weight;
            weight_sum += weight;
        }

    score /= weight_sum;

    if (*best_score <= score)
        return false;

    *best_score = score;
    scoreMap_.setPixel(p, (QRgb)qCeil(score));
    *best_offset = candidate_offset;
    return true;
}
