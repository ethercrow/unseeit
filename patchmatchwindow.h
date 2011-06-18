#ifndef PATCHMATCHWINDOW_H_NKL1CREY
#define PATCHMATCHWINDOW_H_NKL1CREY

#include <QWidget>
#include <QLabel>
#include <QImage>

#include "cowmatrix.h"

class SimilarityMapper;

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

private slots:
    void onIterationComplete(COWMatrix<QPoint> offsetMap,
                             COWMatrix<qreal> reliabilityMap);

private:
    void launch();
    QImage applyOffsetsWeighted(const COWMatrix<QPoint>& offsetMap, const COWMatrix<qreal>& relMap);
    QImage applyOffsetsUnweighted(const COWMatrix<QPoint>& offsetMap);

    QImage* dstImage_;
    QImage* srcImage_;

    QLabel* dstLabel_;
    QLabel* srcLabel_;
    QLabel* offsetLabel_;
    QLabel* errorLabel_;
    QLabel* resultLabel_;

    SimilarityMapper* sm_;
};

#endif /* end of include guard: PATCHMATCHWINDOW_H_NKL1CREY */

