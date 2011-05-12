#include "window.h"

#include <QApplication>
#include <QDebug>
#include <QString>
#include <QPainter>
#include <QResizeEvent>
#include <QBitmap>
#include <qmath.h>

#include "resynthesizer.h"
#include "utils.h"

const int MAX_PAINT_SIZE = 20;
const int MIN_PAINT_SIZE = 1;

Window::Window(QWidget* parent):QGraphicsView(parent),
    pictureImage_(NULL), overlayImage_(NULL),
    paintSize_(3)
{
    setGeometry(0, 0, 1024, 768);
    setAlignment(0);

    scene_ = new QGraphicsScene(this);
    setScene(scene_);
    setMouseTracking(true);

    rootItem_      = new QGraphicsRectItem;
    imageItem_     = new QGraphicsPixmapItem(rootItem_);
    overlayItem_   = new QGraphicsPixmapItem{imageItem_};
    resultItem_    = new QGraphicsPixmapItem(rootItem_);
    offsetMapItem_ = new QGraphicsPixmapItem(rootItem_);
    scoreMapItem_  = new QGraphicsPixmapItem(rootItem_);

    brushItem_ = new QGraphicsPixmapItem(overlayItem_);

    imageItem_->setAcceptHoverEvents(true);
    overlayItem_->setAcceptHoverEvents(true);
    resultItem_->setAcceptHoverEvents(true);
    offsetMapItem_->setAcceptHoverEvents(true);
    scoreMapItem_->setAcceptHoverEvents(true);
    brushItem_->setAcceptHoverEvents(true);

    brushItem_->setOpacity(0.5);

    scene_->addItem(rootItem_);
}

void Window::updateBrush()
{
    QPixmap brush_pixmap(2*paintSize_+1, 2*paintSize_+1);

    QPainter painter(&brush_pixmap);
    painter.fillRect(brush_pixmap.rect(), Qt::black);

    brushItem_->setPixmap(brush_pixmap);
}

void Window::loadImage(const QString& filename)
{
    delete pictureImage_;
    pictureImage_ = new QImage(QImage(filename).convertToFormat(QImage::Format_ARGB32));
    if (!pictureImage_) {
        qDebug() << "loading image failed";
    }
    qDebug() << "format:" << pictureImage_->format();
    imageItem_->setPixmap(QPixmap::fromImage(*pictureImage_));

    delete overlayImage_;
    overlayImage_ = new QImage(pictureImage_->size(), QImage::Format_ARGB32);
    overlayImage_->fill(0);

    QRect frame = pictureImage_->rect();
    imageItem_->setPos(0, 0);
    overlayItem_->setPos(0, 0);
    resultItem_->setPos(frame.width(), 0);
    offsetMapItem_->setPos(frame.width(), frame.height());
    scoreMapItem_->setPos(0, frame.height());
    scene_->setSceneRect(0, 0, frame.width()*2, frame.height()*2);

    updateBrush();
}

void Window::mousePressEvent(QMouseEvent* evt)
{
    if (evt->buttons() & Qt::MiddleButton) {
        prevPan_ = mapToScene(evt->pos());
        setCursor(Qt::ClosedHandCursor);
    }
}

void Window::mouseReleaseEvent(QMouseEvent* evt)
{
    setCursor(Qt::ArrowCursor);
}

void Window::mouseMoveEvent(QMouseEvent* evt)
{
    QPoint draw_location = (mapToScene(evt->pos()) - rootItem_->pos()).toPoint();

    if (evt->buttons() & Qt::LeftButton) {
        for (int j=-paintSize_; j<=paintSize_; ++j)
            for (int i=-paintSize_; i<=paintSize_; ++i)
                overlayImage_->setPixel(draw_location+QPoint(i,j), 0xff000000);
        overlayItem_->setPixmap(QPixmap::fromImage(*overlayImage_));
    } else if (evt->buttons() & Qt::RightButton) {
        for (int j=-paintSize_; j<=paintSize_; ++j)
            for (int i=-paintSize_; i<=paintSize_; ++i)
                overlayImage_->setPixel(draw_location+QPoint(i,j), 0x00000000);
        overlayItem_->setPixmap(QPixmap::fromImage(*overlayImage_));
    } else if (evt->buttons() & Qt::MiddleButton) {
        auto pan_to = mapToScene(evt->pos());
        QPointF dp = pan_to - prevPan_;
        rootItem_->moveBy(dp.x(), dp.y());
        prevPan_ = pan_to;
        return;
    }

    brushItem_->setPos(draw_location - QPoint(paintSize_, paintSize_));
}

void Window::keyReleaseEvent(QKeyEvent* evt)
{
    switch (evt->key()) {
        case Qt::Key_Return: {
            Resynthesizer r;

            QImage result = r.inpaintHier(*pictureImage_, *overlayImage_);
            resultItem_->setPixmap(QPixmap::fromImage(result));

            offsetMapItem_->setPixmap(QPixmap::fromImage(visualizeOffsetMap(r.offsetMap())));
            break;
        }
        case Qt::Key_Space:
            overlayImage_->fill(0);
            overlayItem_->setPixmap(QPixmap::fromImage(*overlayImage_));
            break;
        case Qt::Key_Plus:
            if (paintSize_ < MAX_PAINT_SIZE)
                ++paintSize_;
            updateBrush();
            break;
        case Qt::Key_Minus:
            if (paintSize_ > MIN_PAINT_SIZE)
                --paintSize_;
            updateBrush();
            break;
        case Qt::Key_Q:
            if (evt->modifiers() & Qt::ControlModifier)
                qApp->exit(0);
            break;
    }
}

void Window::wheelEvent(QWheelEvent* evt)
{
    if (evt->modifiers() & Qt::ControlModifier) {
        // zoom workspace
        qreal dscale = pow((double)2, evt->delta()/480.0);
        scale(dscale, dscale);
    } else {
        // scale brush
        paintSize_ = qBound(MIN_PAINT_SIZE, paintSize_+(evt->delta()/120), MAX_PAINT_SIZE);
        updateBrush();
    }
}

Window::~Window()
{
    delete pictureImage_;
    delete overlayImage_;
}

