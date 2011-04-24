#ifndef UNSEEIT_WINDOW_H
#define UNSEEIT_WINDOW_H

#include <QWidget>

#include <QLabel>
#include <QPixmap>

class Window: public QWidget
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
    QLabel* imageLabel_;
    QLabel* overlayLabel_;
    QLabel* resultLabel_;
    QLabel* offsetMapLabel_;
    QLabel* scoreMapLabel_;

    QImage* pictureImage_;
    QImage* overlayImage_;

    int paintSize_;
};

#endif
