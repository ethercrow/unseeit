#include "utils.h"

#include <math.h>

#include <QColor>
#include <qmath.h>
#include <QtGlobal>

QImage downscale_mask(const QImage mask, const QSize dstSize)
{
    TRACE_ME
    QImage result(dstSize, QImage::Format_Mono);

    // assuming aspect preservation
    qreal inv_scale = static_cast<qreal>(mask.width())/dstSize.width();

    for (int j=0; j<result.height(); ++j)
        for (int i=0; i<result.width(); ++i) {
            int x1 = i*inv_scale;
            int x2 = (i+1)*inv_scale;
            int y1 = j*inv_scale;
            int y2 = (j+1)*inv_scale;

            // if any pixel in [x1..x2]x[y1..y2] region of mask is true
            // then pixel (i,j) on downscaled mask becomes true
            int downsampled_value = 0;
            for (int mj=y1; mj<y2; ++mj)
                for (int mi=x1; mi<x2; ++mi)
                    downsampled_value |= mask.pixelIndex(mi, mj);

            result.setPixel(i, j, downsampled_value);
        }

    return result;
}

COWMatrix<QPoint> resize_offset_map(const COWMatrix<QPoint> src, const QSize dstSize)
{
    TRACE_ME

    COWMatrix<QPoint> result(dstSize);

    qreal scale = static_cast<qreal>(dstSize.width())/src.width();
    qreal inv_scale = 1.f/scale;
    int sw = src.width();
    int sh = src.height();

    for (int j=0; j<result.height(); ++j)
        for (int i=0; i<result.width(); ++i) {
            int source_x = qBound(0, qRound(i*inv_scale), sw-1);
            int source_y = qBound(0, qRound(j*inv_scale), sh-1);

            QPoint scaled_offset = src.get(source_x, source_y)*scale;
            result.set(i, j, scaled_offset);
        }

    return result;
}

QImage visualizeOffsetMap(const COWMatrix<QPoint>& offsetMap)
{
    QImage result(offsetMap.size(), QImage::Format_RGB32);

    // angle -> hue
    // magnitude -> saturation
    for (int j=0; j<offsetMap.height(); ++j)
        for (int i=0; i<offsetMap.width(); ++i) {
            QPoint o = offsetMap.get(i, j);
            int hue = 180/M_PI*qAtan2(o.x(), o.y());
            if (hue<0)
                hue += 360;
            if (hue==360)
                hue = 0;

            int value = 128;
            int saturation = 255*sqrt(qreal(o.x()*o.x() + o.y()*o.y())/
                    (offsetMap.width()*offsetMap.width() + offsetMap.height()*offsetMap.height()));
            QColor c = QColor::fromHsv(hue, saturation, value);
            result.setPixel(i, j, c.rgb());
        }

    return result;
}

