#include "window.h"
#include "resynthesizer.h"

#include <QDebug>
#include <QString>
#include <QResizeEvent>

Window::Window(QWidget* parent):QWidget(parent),
    pictureImage_(NULL), overlayImage_(NULL),
    paintSize_(3)
{
    imageLabel_ = new QLabel(this);
    overlayLabel_ = new QLabel(this);
    resultLabel_ = new QLabel(this);
}

void Window::loadImage(const QString& filename)
{
    delete pictureImage_;
    pictureImage_ = new QImage(QImage(filename).convertToFormat(QImage::Format_ARGB32));
    if (!pictureImage_) {
        qDebug() << "loading image failed";
    }
    qDebug() << "format:" << pictureImage_->format();
    imageLabel_->setPixmap(QPixmap::fromImage(*pictureImage_));

    delete overlayImage_;
    overlayImage_ = new QImage(pictureImage_->size(), QImage::Format_ARGB32);
    overlayImage_->fill(0);

    QRect frame = QRect(0, 0, pictureImage_->width(), pictureImage_->height());
    imageLabel_->setGeometry(frame);
    overlayLabel_->setGeometry(frame);
    resultLabel_->setGeometry(frame.translated(frame.width(), 0));

    this->resize(frame.width()*2, frame.height());
}

void Window::mouseMoveEvent(QMouseEvent* evt)
{
    QPoint location = evt->pos();

    for (int j=-paintSize_; j<=paintSize_; ++j)
        for (int i=-paintSize_; i<=paintSize_; ++i)
            overlayImage_->setPixel(location+QPoint(i,j), 0xff000000);

    overlayLabel_->setPixmap(QPixmap::fromImage(*overlayImage_));
}

void Window::keyReleaseEvent(QKeyEvent* evt)
{
    switch (evt->key()) {
        case Qt::Key_Return: {
            Resynthesizer r;
            QImage result = r.inpaint(*pictureImage_, *overlayImage_);
            resultLabel_->setPixmap(QPixmap::fromImage(result));
            break;
        }
        case Qt::Key_Space:
            overlayImage_->fill(0);
            overlayLabel_->setPixmap(QPixmap::fromImage(*overlayImage_));
            break;
    }
}

Window::~Window()
{
    delete pictureImage_;
    delete overlayImage_;
}
