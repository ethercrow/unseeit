#include "similaritymapper.h"

#include <algorithm>
#include <iostream>
#include <qmath.h>

#include "pixel.h"
#include "randomoffsetgenerator.h"
#include "utils.h"

const int R = 4;
const int PASS_COUNT = 12;

// this value can be varied from "omg blurry" to "wtf is that?!"
const double SIGMA2 = (2*R+1)*(2*R+1)*0.2f;

void SimilarityMapper::init(const QImage& src, const QImage& dst)
{
    TRACE_ME

    mode_ = SMModeSimple;

    // offsetmap has the same dimensions as dst
    offsetMap_ = COWMatrix<QPoint>(dst.size());
    src_ = src;
    auto srcMask = QImage(src.size(), QImage::Format_Mono);
    srcMask.fill(1);

    scoreMap_ = COWMatrix<int>(dst.size());
    scoreMap_.fill(INT_MAX);
    reliabilityMap_ = COWMatrix<qreal>(scoreMap_.size(), 0.0);

    // fill offsetmap with random offsets for unknows points
    RandomOffsetGenerator rog(srcMask, R);
    for (int j=0; j<offsetMap_.height(); ++j)
        for (int i=0; i<offsetMap_.width(); ++i)
            offsetMap_.set(i, j, rog(i, j));

    // create list of unknown points
    for (int j=R; j<dst.height()-R; ++j) {
        for (int i=R, i_end = dst.width()-R; i<i_end; ++i)
            pointsToFill_ << QPoint(i, j);

        for (int i=dst.width()-1; i >= 0; --i)
            reversePointsToFill_ << QPoint(i, j);
    }

    qDebug() << pointsToFill_.size() << "points to map";
}

void SimilarityMapper::init(const QImage& src,
        const QImage& dst, const QImage& srcMask, const QImage& dstMask)
{
    TRACE_ME

    mode_ = SMModeMasked;

    // offsetmap has the same dimensions as dst
    offsetMap_ = COWMatrix<QPoint>(dst.size());
    src_ = src;
    srcMask_ = srcMask;
    dstMask_ = dstMask;

    scoreMap_ = COWMatrix<int>(dst.size());
    scoreMap_.fill(0);
    reliabilityMap_ = COWMatrix<qreal>(scoreMap_.size(), 1.0);

    // fill offsetmap with random offsets for unknows points
    offsetMap_.fill(QPoint(0, 0));
    RandomOffsetGenerator rog(srcMask, R);
    for (int j=0; j<offsetMap_.height(); ++j)
        for (int i=0; i<offsetMap_.width(); ++i)
            if (!dstMask.pixelIndex(i, j)) {
                offsetMap_.set(i, j, rog(i, j));
                scoreMap_.set(i, j, INT_MAX);
                reliabilityMap_.set(i, j, 0.0);
            }

    // create list of unknown points
    for (int j=R; j<dst.height()-R; ++j) {
        for (int i=R, i_end = dst.width()-R; i<i_end; ++i)
            if (!dstMask.pixelIndex(i, j))
                pointsToFill_ << QPoint(i, j);

        for (int i=dst.width()-1; i >= 0; --i)
            if (!dstMask.pixelIndex(i, j))
                reversePointsToFill_ << QPoint(i, j);
    }

    qDebug() << pointsToFill_.size() << "points to map";
}

COWMatrix<QPoint> SimilarityMapper::iterate(const QImage& dst)
{
    TRACE_ME

    dst_ = dst;
    int init_search_range = qMax(src_.width(), src_.height());

    QVector<QPolygon> neighbour_offsets_passes(4);
    neighbour_offsets_passes[0] << QPoint(0, -1) << QPoint(-1, 0);
    neighbour_offsets_passes[1] << QPoint(0,  1) << QPoint(-1, 0);
    neighbour_offsets_passes[2] << QPoint(0,  1) << QPoint( 1, 0);
    neighbour_offsets_passes[3] << QPoint(0, -1) << QPoint( 1, 0);

    for (int pass=0; pass<PASS_COUNT; ++pass) {
        // refine pass
        // there are two kinds of places where we can look for better matches:
        // 1. Obviously, random places
        // 2. Propagation: trying points near our neighbour's source

        const QPolygon& neighbour_offsets = neighbour_offsets_passes[pass%4];
        const QPolygon& points = (pass%2)?reversePointsToFill_:pointsToFill_;

        // random search
        foreach(QPoint p, points) {
            QPoint best_offset = offsetMap_.get(p);
            int best_score = scoreMap_.get(p);

            if (best_score == 0)
                continue;

            for (int range=init_search_range; range>0; range/=2) {
                QPoint o(best_offset);
                o.rx() += qrand()%(2*range) - range;
                o.ry() += qrand()%(2*range) - range;
                updateSource(p, &best_offset, o, &best_score);
            }

            // save found offset
            offsetMap_.set(p, best_offset);
        }

        // propagate good guess
        foreach(QPoint p, points) {
            QPoint best_offset = offsetMap_.get(p);
            int best_score = scoreMap_.get(p);

            if (best_score == 0)
                continue;

            foreach (QPoint dp, neighbour_offsets) {
                QPoint pdp = p+dp;
                if (pdp.x() >= 0 && pdp.y() >= 0 && (SMModeSimple == mode_ || (
                    pdp.x() < dstMask_.width() && pdp.y() < dstMask_.height() &&
                    !dstMask_.pixelIndex(pdp))))
                {
                    // our neighbour is unknown point too
                    // maybe his offset is better than ours
                    QPoint neighbours_offset = offsetMap_.get(p+dp);
                    if (scoreMap_.get(p+dp) - 4*R*SIGMA2 < scoreMap_.get(p))
                        updateSource(p, &best_offset, neighbours_offset, &best_score);
                }
            }

            // save found offset
            offsetMap_.set(p, best_offset);
        }
        if (pass%2) {
            std::reverse(pointsToFill_.begin(), pointsToFill_.end());
            std::reverse(reversePointsToFill_.begin(), reversePointsToFill_.end());
        }

        emit iterationComplete(offsetMap_, reliabilityMap_);
    }

    report_max_score();

    return offsetMap_;
}

void SimilarityMapper::report_max_score()
{
    maxScore_ = 0;
    int min_score = INT_MAX;
    meanScore_ = 0;

    foreach(QPoint p, pointsToFill_) {
        int score = scoreMap_.get(p);
        maxScore_ = qMax(maxScore_, score);
        min_score = qMin(min_score, score);
        meanScore_ += score;
    }

    meanScore_ /= pointsToFill_.size();

    qDebug() << "score : max =" << maxScore_
        << "mean =" << meanScore_
        << "min =" << min_score;
}

bool SimilarityMapper::updateSource(QPoint p, QPoint* best_offset,
    QPoint candidate_offset, int* best_score)
{
    if (SMModeSimple == mode_)
        return updateSourceSimple(p, best_offset, candidate_offset, best_score);
    else
        return updateSourceMasked(p, best_offset, candidate_offset, best_score);
}

bool SimilarityMapper::updateSourceMasked(QPoint p, QPoint* best_offset,
    QPoint candidate_offset, int* best_score)
{
    QRect bounds(QPoint(0, 0), src_.size());

    int dw = dst_.width();
    int dh = dst_.height();

    int sw = src_.width();
    int sh = src_.height();

    // source point
    QPoint s = p + candidate_offset;

    // fuck the edge cases
    if (p.x() < R || p.x() >= dw-R || p.y() < R || p.y() >= dh-R ||
        s.x() < R || s.x() >= sw-R || s.y() < R || s.y() >= sh-R)
        return false;

    // we need only true real patches as sources
    if (!srcMask_.pixelIndex(s))
        return false;

    if (!bounds.contains(p) || !bounds.contains(s))
        return false;

    double score = 0;
    double weight_sum = 0;

    const qreal* weight_ptr = reliabilityMap_.ptrAt(p-QPoint(R,R));
    QRgb* ns_pixel_ptr = reinterpret_cast<QRgb*>(src_.bits()) + (s.y()-R)*sw + (s.x()-R);
    QRgb* np_pixel_ptr = reinterpret_cast<QRgb*>(dst_.bits()) + (p.y()-R)*dw + (p.x()-R);
    for (int j=-R; j<=R; ++j) {
        for (int i=-R; i<=R; ++i) {
            quint8* ns_rgb = reinterpret_cast<quint8*>(ns_pixel_ptr);
            quint8* np_rgb = reinterpret_cast<quint8*>(np_pixel_ptr);

            score += ssd4(ns_rgb, np_rgb)*(*weight_ptr);
            weight_sum += *weight_ptr;

            ++weight_ptr;
            ++ns_pixel_ptr;
            ++np_pixel_ptr;
        }
        weight_ptr += dw - 2*R - 1;
        ns_pixel_ptr += sw - 2*R - 1;
        np_pixel_ptr += dw - 2*R - 1;
    }

    score /= weight_sum;

    if (*best_score <= score)
        return false;

    *best_score = score;
    scoreMap_.set(p, qCeil(score));
    reliabilityMap_.set(p, qExp(-score/SIGMA2));
    *best_offset = candidate_offset;

    return true;
}

bool SimilarityMapper::updateSourceSimple(QPoint p, QPoint* best_offset,
    QPoint candidate_offset, int* best_score)
{
    int dw = dst_.width();
    int dh = dst_.height();

    int sw = src_.width();
    int sh = src_.height();

    // source point
    QPoint s = p + candidate_offset;

    // fuck the edge cases
    if (p.x() < R || p.x() >= dw-R || p.y() < R || p.y() >= dh-R ||
        s.x() < R || s.x() >= sw-R || s.y() < R || s.y() >= sh-R)
        return false;

    int score = 0;

    QRgb* ns_pixel_ptr = reinterpret_cast<QRgb*>(src_.bits()) + (s.y()-R)*sw + (s.x()-R);
    QRgb* np_pixel_ptr = reinterpret_cast<QRgb*>(dst_.bits()) + (p.y()-R)*dw + (p.x()-R);
    for (int j=-R; j<=R; ++j) {
        for (int i=-R; i<=R; ++i) {
            quint8* ns_rgb = reinterpret_cast<quint8*>(ns_pixel_ptr);
            quint8* np_rgb = reinterpret_cast<quint8*>(np_pixel_ptr);

            score += ssd4(ns_rgb, np_rgb);

            if (*best_score <= score)
                return false;

            ++ns_pixel_ptr;
            ++np_pixel_ptr;
        }
        ns_pixel_ptr += sw - 2*R - 1;
        np_pixel_ptr += dw - 2*R - 1;
    }

    *best_score = score;
    scoreMap_.set(p, score);
    reliabilityMap_.set(p, qExp(-score/SIGMA2));
    *best_offset = candidate_offset;

    return true;
}
