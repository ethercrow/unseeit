#ifndef UNSEEIT_RESYNTHESIZER_H
#define UNSEEIT_RESYNTHESIZER_H

#include <QImage>
#include <QPoint>
#include <QVector>

#include "cowmatrix.h"

class Resynthesizer
{

public:
    QImage inpaint(const QImage& inputTexture, const QImage& outputMap);
    COWMatrix<QPoint> buildOffsetMap(const QImage& inputTexture,
                          const QImage& outputMap,
                          const COWMatrix<QPoint>& hint);
    QImage inpaintHier(const QImage& inputTexture, const QImage& outputMap);

    COWMatrix<QPoint> offsetMap();

private:
    int coherence(QPoint p1, QPoint p2, int R);
    void mergePatches(bool weighted);

    // TODO: stop using QVector and QImage as matrices ffs
    //       oh wow, there's some progress on that

    QVector<qreal> confidenceMap_;
    const QImage* inputTexture_;
    const COWMatrix<int>* scoreMap_;
    QImage outputTexture_;
    COWMatrix<QPoint> offsetMap_;

    QImage realMap_;
};

#endif
