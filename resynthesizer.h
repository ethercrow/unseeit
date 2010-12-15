#ifndef UNSEEIT_RESYNTHESIZER_H
#define UNSEEIT_RESYNTHESIZER_H

#include <QImage>
#include <QPoint>

class Resynthesizer
{

public:
    QImage inpaint(const QImage& inputTexture, const QImage& outputMap);

    QImage offsetMap();
    QImage realMap();

private:
    int coherence(QPoint p1, QPoint p2, int R);
    void mergePatches();

    const QImage* inputTexture_;
    QImage outputTexture_;
    QImage offsetMap_;

    QImage realMap_;
};

#endif
