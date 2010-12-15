#ifndef UNSEEIT_UTILS_H
#define UNSEEIT_UTILS_H

#include <QDebug>
#include <QString>
#include <QPoint>
#include <QImage>

struct ScopeTracer
{
    ScopeTracer(const QString& name):name_(name) {
        qDebug() << name_ << " begin";
    }
    ~ScopeTracer() {
        qDebug() << name_ << " end";
    }
private:
    QString name_;
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

#endif
