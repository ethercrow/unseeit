#ifndef UNSEEIT_WINDOW_H
#define UNSEEIT_WINDOW_H

#include <QGraphicsView>

#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QGraphicsScene>

class Window: public QGraphicsView
{
    Q_OBJECT

public:
    Window(QWidget* parent = 0);
    ~Window();

    void loadImage(const QString& filename);

protected:
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void keyReleaseEvent(QKeyEvent*);
    virtual void wheelEvent(QWheelEvent*);

private:
    QGraphicsScene* scene_;

    QGraphicsPixmapItem* imageItem_;
    QGraphicsPixmapItem* overlayItem_;
    QGraphicsPixmapItem* resultItem_;
    QGraphicsPixmapItem* offsetMapItem_;
    QGraphicsPixmapItem* scoreMapItem_;

    QGraphicsPixmapItem* brushItem_;

    QImage* pictureImage_;
    QImage* overlayImage_;

    int paintSize_;

    void updateBrush();
};

#endif
