#ifndef UNSEEIT_RESYNTHESIZER_H
#define UNSEEIT_RESYNTHESIZER_H

#include <QImage>
#include <QPoint>
#include <QVector>

#include "cowmatrix.h"

class Resynthesizer
{

public:
    COWMatrix<QPoint> buildOffsetMap(const QImage& inputTexture,
                          const QImage& outputMap,
                          const COWMatrix<QPoint>& hint);
    QImage inpaintHier(const QImage& inputTexture, const QImage& outputMap);

    COWMatrix<QPoint> offsetMap() { return offsetMap_; }
    COWMatrix<qreal> reliabilityMap() { return reliabilityMap_; }

private:
    int coherence(QPoint p1, QPoint p2, int R);
    void mergePatches(bool weighted);

    // TODO: stop using QVector and QImage as matrices ffs
    //       oh wow, there's some progress on that

    QVector<qreal> confidenceMap_;
    const QImage* inputTexture_;
    QImage outputTexture_;

    COWMatrix<QPoint> offsetMap_;
    COWMatrix<qreal> reliabilityMap_;

    QImage realMap_;
};

#endif
