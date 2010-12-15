#ifndef UNSEEIT_SIMILARITYMAPPER_H
#define UNSEEIT_SIMILARITYMAPPER_H

#include <QImage>

class SimilarityMapper
{
public:
    // src, dst - argb32, src_mask - mono
    QImage calculateVectorMap(const QImage& src, const QImage& dst, const QImage& srcMask);

private:
    bool updateSource(QPoint p, QPoint* current_offset,
        QPoint candidate_offset, int* score);

    QImage offsetMap_;
    const QImage* src_;
    const QImage* dst_;
    const QImage* srcMask_;
};

#endif
