#include <QImage>
#include <QPoint>

class Resynthesizer
{

public:
    QImage inpaint(const QImage& inputTexture, const QImage& outputMap);

private:
    bool updateSource(QPoint p, QPoint* current_offset,
        QPoint candidate_offset, int* score);

    const QImage* inputTexture_;
    const QImage* outputMap_;
    QImage outputTexture_;
};
