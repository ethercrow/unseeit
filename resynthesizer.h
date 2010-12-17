#ifndef UNSEEIT_RESYNTHESIZER_H
#define UNSEEIT_RESYNTHESIZER_H

#include <QImage>
#include <QPoint>
#include <QVector>

class Resynthesizer
{

public:
    QImage inpaint(const QImage& inputTexture, const QImage& outputMap);

    QImage offsetMap();

private:
    int coherence(QPoint p1, QPoint p2, int R);
    void mergePatches(bool weighted);

    QVector<qreal> confidenceMap_;
    const QImage* inputTexture_;
    const QImage* scoreMap_;
    QImage outputTexture_;
    QImage offsetMap_;

    QImage realMap_;
};

#endif
