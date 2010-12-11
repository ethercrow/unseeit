#ifndef UNSEEIT_UTILS_H
#define UNSEEIT_UTILS_H

#include <QDebug>
#include <QString>

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

#endif
