#ifndef COWMATRIX_H_OSZHYVUP
#define COWMATRIX_H_OSZHYVUP

#include <QVector>
#include <QPoint>

template <typename T>
class COWMatrix
{
public:
    COWMatrix(): w_(0), h_(0) {}

    COWMatrix(int w, int h): w_(w), h_(h), data_(w*h)
    {
    }

    COWMatrix(const QSize& sz): w_(sz.width()), h_(sz.height()), data_(w_*h_)
    {
    }

    T* ptrAt(int i, int j) {
        return &data_[j*w_+i];
    }

    T* ptrAt(const QPoint& p) {
        return ptrAt(p.x(), p.y());
    }

    void fill(const T& value) {
        data_.fill(value);
    }

    const T& get(int i, int j) const {
        return data_[j*w_+i];
    }

    const T& get(const QPoint& p) const {
        return get(p.x(), p.y());
    }

    void set(int i, int j, const T& value) {
        data_[j*w_+i] = value;
    }

    void set(const QPoint& p, const T& value) {
        set(p.x(), p.y(), value);
    }

    QSize size() const { return QSize(w_, h_); }
    int width() const { return w_; }
    int height() const { return h_; }

    bool isNull() const {
        return !(w_ && h_);
    }

private:
    int w_;
    int h_;
    QVector<T> data_;
};

#endif /* end of include guard: COWMATRIX_H_OSZHYVUP */

