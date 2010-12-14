#include <QImage>
#include <QPoint>

class Resynthesizer
{

public:
    QImage inpaint(const QImage& inputTexture, const QImage& outputMap);

    QImage offsetMap();
    QImage scoreMap();

private:
    bool updateSource(QPoint p, QPoint* current_offset,
        QPoint candidate_offset, int* score);
    int coherence(QPoint p1, QPoint p2, int R);

    const QImage* inputTexture_;
    const QImage* outputMap_;
    QImage outputTexture_;
    QImage offsetMap_;
    QImage scoreMap_;

    QImage realMap_;
};
