#include "patchmatchwindow.h"

#include <QKeyEvent>
#include <QtConcurrentRun>

#include "similaritymapper.h"
#include "utils.h"

#include <qmath.h>

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
    errorLabel_->setGeometry(500, 500, 500, 500);

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

void PatchMatchWindow::onIterationComplete(COWMatrix<QPoint> offsetMap,
                                           COWMatrix<qreal> reliabilityMap)
{
    TRACE_ME

    QImage offsetMapVisual = visualizeOffsetMap(offsetMap);
    offsetLabel_->setPixmap(QPixmap::fromImage(offsetMapVisual));

    QImage resultImage = applyOffsetsWeighted(offsetMap, reliabilityMap);
    resultLabel_->setPixmap(QPixmap::fromImage(resultImage));

    QImage errorImage = visualizeReliabilityMap(reliabilityMap);
    errorLabel_->setPixmap(QPixmap::fromImage(errorImage));

    update();
}

void PatchMatchWindow::launch()
{
    TRACE_ME

    qRegisterMetaType<COWMatrix<QPoint>>("COWMatrix<QPoint>");
    qRegisterMetaType<COWMatrix<qreal>>("COWMatrix<qreal>");

    sm_ = new SimilarityMapper;

    connect(sm_, SIGNAL(iterationComplete(COWMatrix<QPoint>, COWMatrix<qreal>)),
            this, SLOT(onIterationComplete(COWMatrix<QPoint>, COWMatrix<qreal>)));

    sm_->init(*srcImage_, *dstImage_);
    QFuture<COWMatrix<QPoint>> offsetMapFuture = QtConcurrent::run(sm_, &SimilarityMapper::iterate, *dstImage_);

    // auto offsetMap = offsetMapFuture.result();
    // QImage offsetMapVisual = visualizeOffsetMap(offsetMap);
    // offsetLabel_->setPixmap(QPixmap::fromImage(offsetMapVisual));

    // //QImage resultImage = applyOffsetsWeighted(offsetMap, sm.scoreMap());
    // QImage resultImage = applyOffsetsUnweighted(offsetMap);
    // resultLabel_->setPixmap(QPixmap::fromImage(resultImage));
    // update();
}

QImage PatchMatchWindow::applyOffsetsWeighted(const COWMatrix<QPoint>& offsetMap, const COWMatrix<qreal>& relMap)
{
    QImage result{offsetMap.size(), QImage::Format_RGB32};

    QRect bounds = result.rect();
    int width  = offsetMap.width();
    int height = offsetMap.height();

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

                    qreal weight = relMap.get(near_p);

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

QImage PatchMatchWindow::applyOffsetsUnweighted(const COWMatrix<QPoint>& offsetMap)
{
    QImage result{offsetMap.size(), QImage::Format_RGB32};

    QRect bounds = result.rect();
    int width  = offsetMap.width();
    int height = offsetMap.height();

    for (int j=0; j<height; ++j)
        for (int i=0; i<width; ++i) {
            QPoint p(i, j);

            // p - provider_for_p, necessary if p is near edge
            QPoint dp(0, 0);

            if (i<R)
               dp.rx() = R-i;
            else if (i >= width-R)
               dp.rx() = width-R-1-i;

            if (j<R)
               dp.ry() = R-j;
            else if (j >= height-R)
               dp.ry() = height-R-1-j;

            QPoint source = p + offsetMap.get(p+dp);

            result.setPixel(p, srcImage_->pixel(source));
        }

    return result;
}
