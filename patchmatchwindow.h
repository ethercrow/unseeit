#ifndef PATCHMATCHWINDOW_H_NKL1CREY
#define PATCHMATCHWINDOW_H_NKL1CREY

#include <QWidget>
#include <QLabel>
#include <QImage>

class PatchMatchWindow: public QWidget
{
    Q_OBJECT

public:
    PatchMatchWindow(QWidget* parent=0);
    ~PatchMatchWindow();

    void loadDst(QString filename);
    void loadSrc(QString filename);

protected:
    void keyReleaseEvent(QKeyEvent* evt);

private:
    void launch();
    QImage applyOffsets(const QImage& offsetMap);

    QImage* dstImage_;
    QImage* srcImage_;

    QLabel* dstLabel_;
    QLabel* srcLabel_;
    QLabel* offsetLabel_;
    QLabel* errorLabel_;
    QLabel* resultLabel_;
};

#endif /* end of include guard: PATCHMATCHWINDOW_H_NKL1CREY */

