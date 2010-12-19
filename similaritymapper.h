#ifndef UNSEEIT_SIMILARITYMAPPER_H
#define UNSEEIT_SIMILARITYMAPPER_H

#include <QImage>
#include <QPolygon>

class SimilarityMapper
{
public:
    // src, dst - argb32, src_mask - mono
    void init(const QImage& src, const QImage& dst, const QImage& srcMask, const QImage& dstMask);
    QImage iterate(const QImage& dst);

    const QImage* scoreMap() { return &scoreMap_; };
    const QVector<double>* reliabilityMap() { return &reliabilityMap_; };
    const QVector<qreal> confidenceMap();

private:
    bool updateSource(QPoint p, QPoint* current_offset,
        QPoint candidate_offset, int* score);
    void report_max_score();

    // reliability = exp(-score/SIGMA2);
    QImage scoreMap_;
    QVector<qreal> reliabilityMap_;

    QImage offsetMap_;
    const QImage* src_;
    QImage dst_;
    const QImage* srcMask_;
    const QImage* dstMask_;

    QPolygon pointsToFill_;
};

#endif
