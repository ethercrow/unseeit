#include "patchmatchwindow.h"

#include <QKeyEvent>

#include "similaritymapper.h"
#include "utils.h"

PatchMatchWindow::PatchMatchWindow(QWidget* parent): QWidget(parent)
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

    QImage resultImage = applyOffsets(offsetMap);
    resultLabel_->setPixmap(QPixmap::fromImage(resultImage));
    update();
}

QImage PatchMatchWindow::applyOffsets(const QImage& offsetMap)
{
    TRACE_ME

    auto result = *dstImage_;

    for (int j=0; j<result.height(); ++j)
        for (int i=0; i<result.width(); ++i) {
            QPoint o = rgb_to_point(offsetMap.pixel(i, j));
            result.setPixel(i, j, srcImage_->pixel(QPoint(i, j) + o));
        }

    return result;
}
