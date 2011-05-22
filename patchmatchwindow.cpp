#include "patchmatchwindow.h"

#include <QKeyEvent>

#include "similaritymapper.h"
#include "utils.h"

#include <qmath.h>

const double SIGMA2 = 20.f;
const int R = 4;

PatchMatchWindow::PatchMatchWindow(QWidget* parent): QWidget(parent),
    srcImage_(NULL), dstImage_(NULL)
{
    srcLabel_ = new QLabel(this);
    srcLabel_->setGeometry(0, 0, 500, 500);

    dstLabel_ = new QLabel(this);
    dstLabel_->setGeometry(500, 0, 500, 500);

    offsetLabel_ = new QLabel(this);
    offsetLabel_->setGeometry(1000, 0, 500, 500);

    errorLabel_ = new QLabel(this);
    errorLabel_->setGeometry(1000, 500, 500, 500);

    resultLabel_ = new QLabel(this);
    resultLabel_->setGeometry(1000, 500, 500, 500);
}

PatchMatchWindow::~PatchMatchWindow()
{
    delete srcImage_;
    delete dstImage_;
}

void PatchMatchWindow::loadDst(QString filename)
{
    dstImage_ = new QImage(filename);
    dstLabel_->setPixmap(QPixmap::fromImage(*dstImage_));
}

void PatchMatchWindow::loadSrc(QString filename)
{
    srcImage_ = new QImage(filename);
    srcLabel_->setPixmap(QPixmap::fromImage(*srcImage_));
}

void PatchMatchWindow::keyReleaseEvent(QKeyEvent* evt)
{
    switch (evt->key()) {
        case Qt::Key_Return:
            launch();
            break;
    }
}

void PatchMatchWindow::launch()
{
    TRACE_ME

    SimilarityMapper sm;

    sm.init(*srcImage_, *dstImage_);
    auto offsetMap = sm.iterate(*dstImage_);
    QImage offsetMapVisual = visualizeOffsetMap(offsetMap); 
    offsetLabel_->setPixmap(QPixmap::fromImage(offsetMapVisual));

    QImage resultImage = applyOffsetsWeighted(offsetMap, sm.scoreMap());
    resultLabel_->setPixmap(QPixmap::fromImage(resultImage));
    update();
}

QImage PatchMatchWindow::applyOffsetsWeighted(const COWMatrix<QPoint>& offsetMap, const COWMatrix<int>* scoreMap)
{
    QImage result{offsetMap.size(), QImage::Format_RGB32};

    QRect bounds = result.rect();
    int width  = offsetMap.width();
    int height = offsetMap.height();

    QVector<qreal> weightMap(height*width, 1.0);

    for (int j=0; j<height; ++j)
        for (int i=0; i<width; ++i) {
            QPoint p(i, j);
            int score = scoreMap->get(p);
            qreal reliability = qExp(-score/SIGMA2);
            weightMap[j*width+i] = reliability;
        }

    for (int j=0; j<height; ++j)
        for (int i=0; i<width; ++i) {
            QPoint p(i, j);
            qreal r = 0.0, g = 0.0, b = 0.0;
            qreal new_confidence = 0.0;
            qreal confidence_sum = 0.0;
            qreal weight_sum = 0.0;

            for (int dj=-R; dj<=R; ++dj)
                for (int di=-R; di<=R; ++di) {
                    QPoint near_p = p + QPoint(di, dj);
                    if (!bounds.contains(near_p))
                        continue;

                    QPoint opinion_point = p + offsetMap.get(near_p);

                    QColor c(srcImage_->pixel(opinion_point));

                    int idx = (near_p).y()*width + (near_p).x();
                    qreal weight = (weightMap[idx]);

                    new_confidence += weight;

                    r += c.red()*weight;
                    g += c.green()*weight;;
                    b += c.blue()*weight;;

                    weight_sum += weight;
                }

            r /= weight_sum;
            g /= weight_sum;
            b /= weight_sum;

            result.setPixel(p, QColor(r, g, b).rgb());

        }

    return result;
}
