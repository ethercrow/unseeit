#ifndef UNSEEIT_RANDOM_OFFSET_GENERATOR_H
#define UNSEEIT_RANDOM_OFFSET_GENERATOR_H

#include <QImage>

struct RandomOffsetGenerator
{

    RandomOffsetGenerator(const QImage& realMap, int r);

    QPoint operator()(QPoint p);
    QPoint operator()(int x, int y);

private:
    int width_;
    int height_;
    QImage bitmap_;
};

#endif
