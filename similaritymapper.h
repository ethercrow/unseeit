#ifndef UNSEEIT_SIMILARITYMAPPER_H
#define UNSEEIT_SIMILARITYMAPPER_H

#include <QImage>
#include <QPolygon>
#include "cowmatrix.h"

class SimilarityMapper
{
public:
    // src, dst - argb32, src_mask - mono
    void init(const QImage& src, const QImage& dst, const QImage& srcMask, const QImage& dstMask);
    void init(const QImage& src, const QImage& dst);
    COWMatrix<QPoint> iterate(const QImage& dst);

    const COWMatrix<int>* scoreMap() const { return &scoreMap_; };
    const QVector<double>* reliabilityMap() const { return &reliabilityMap_; };
    const QVector<qreal> confidenceMap() const;

    double meanScore() const { return meanScore_; }

private:
    bool updateSource(QPoint p, QPoint* current_offset,
        QPoint candidate_offset, int* score);
    void report_max_score();

    COWMatrix<int> scoreMap_;

    // reliability = exp(-score/SIGMA2);
    QVector<qreal> reliabilityMap_;

    COWMatrix<QPoint> offsetMap_;

    QPolygon reversePointsToFill_;
    QPolygon pointsToFill_;

    // read-only
    QImage dst_;
    QImage src_;
    QImage srcMask_;
    QImage dstMask_;

    double meanScore_;
};

#endif
