#ifndef UNSEEIT_UTILS_H
#define UNSEEIT_UTILS_H

#include <QDebug>
#include <QString>
#include <QPoint>
#include <QImage>
#include <QTime>

#include "cowmatrix.h"

struct ScopeTracer
{
    ScopeTracer(const QString& name):name_(name), time_() {
        qDebug() << name_ << " begin";
        time_.start();
    }
    ~ScopeTracer() {
        qDebug() << name_ << " end" << time_.elapsed();
    }
private:
    QString name_;
    QTime time_;
};

#define TRACE_ME \
    ScopeTracer func_tracer(__PRETTY_FUNCTION__);


// QPoint <-> QRgb conversions

inline QPoint rgb_to_point(QRgb data) {
    qint16* tmp = reinterpret_cast<qint16*>(&data);
    return QPoint(*tmp, *(tmp+1));
}

inline QRgb point_to_rgb(QPoint point) {
    QRgb result;
    *(reinterpret_cast<qint16*>(&result)) = point.x();
    *(reinterpret_cast<qint16*>(&result) + 1) = point.y();
    return result;
}

// upscale and downscale routines

QImage downscale_mask(const QImage mask, const QSize dstSize);

COWMatrix<QPoint> resize_offset_map(const COWMatrix<QPoint> src, const QSize dstSize);

QImage visualizeOffsetMap(const COWMatrix<QPoint>& offsetMap);

#endif
