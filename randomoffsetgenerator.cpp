#include "randomoffsetgenerator.h"

RandomOffsetGenerator::RandomOffsetGenerator(const QImage& realMap, int r):
    width_(realMap.width()),
    height_(realMap.height()),
    bitmap_(realMap) {
    for (int d=0; d<r; ++d) {
        for (int i=0; i<bitmap_.width(); ++i) {
            bitmap_.setPixel(i, d, 0);
            bitmap_.setPixel(i, bitmap_.height()-d-1, 0);
        }
        for (int j=0; j<bitmap_.height(); ++j) {
            bitmap_.setPixel(d, j, 0);
            bitmap_.setPixel(bitmap_.width()-d-1, j, 0);
        }
    }
}

QPoint RandomOffsetGenerator::operator()(QPoint p) {
    int rand_x, rand_y;

    do {
        rand_x = qrand()%width_;
        rand_y = qrand()%height_;
    } while (!bitmap_.pixelIndex(rand_x, rand_y));

    return QPoint(rand_x, rand_y) - p;
}

QPoint RandomOffsetGenerator::operator()(int x, int y) {
    return operator()(QPoint(x, y));
}

