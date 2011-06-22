#ifndef UNSEEIT_SIMILARITYMAPPER_H
#define UNSEEIT_SIMILARITYMAPPER_H

#include <QImage>
#include <QPolygon>
#include "cowmatrix.h"

enum SimilarityMapperMode
{
    SMModeSimple,
    SMModeMasked
};

struct RandomSearchResult
{
    QPoint point;
    QPoint offset;
    int score;
};


class SimilarityMapper: public QObject
{
    Q_OBJECT

public:
    // src, dst - argb32, src_mask - mono
    void init(const QImage& src, const QImage& dst, const QImage& srcMask, const QImage& dstMask);
    void init(const QImage& src, const QImage& dst);
    COWMatrix<QPoint> iterate(const QImage& dst);

    const COWMatrix<int>* scoreMap() const { return &scoreMap_; };
    const COWMatrix<double> reliabilityMap() const { return reliabilityMap_; };
    const QVector<qreal> confidenceMap() const;

    double meanScore() const { return meanScore_; }
    int maxScore() const { return maxScore_; }

signals:
    void iterationComplete(COWMatrix<QPoint>, COWMatrix<qreal>);

private:
    bool updateSource(QPoint p, QPoint* current_offset,
        QPoint candidate_offset, int* score) const;
    bool updateSourceSimple(QPoint p, QPoint* current_offset,
        QPoint candidate_offset, int* score) const;
    bool updateSourceMasked(QPoint p, QPoint* current_offset,
        QPoint candidate_offset, int* score) const;
    void report_max_score();

    QVector<RandomSearchResult> performRandomSearchForRange(QPolygon points) const;
    RandomSearchResult randomSearchKernel(QPoint p) const;

    COWMatrix<int> scoreMap_;

    // reliability = exp(-score/SIGMA2);
    COWMatrix<qreal> reliabilityMap_;

    COWMatrix<QPoint> offsetMap_;

    QPolygon reversePointsToFill_;
    QPolygon pointsToFill_;

    // read-only
    QImage dst_;
    QImage src_;
    QImage srcMask_;
    QImage dstMask_;

    double meanScore_;
    int maxScore_;

    SimilarityMapperMode mode_;

    int initSearchRange_;
};

#endif
