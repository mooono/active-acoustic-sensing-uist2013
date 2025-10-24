#ifndef SVMCLASSIFIER_H
#define SVMCLASSIFIER_H

#include <QObject>
#include "svm.h"
#include <QVector>
#include <QMap>
#include <QPointF>
#include <QDebug>
#include <QTimer>
#include <QFileDialog>
#include <QProgressBar>

class SVMClassifier : public QObject
{
    Q_OBJECT
public:
    explicit SVMClassifier(QObject *parent = 0);

public slots:
    void train(QList<QPair<double, QVector<float> > > _problems);
    double predict(QVector<float> data, double *probability = NULL);
    void setParam(svm_parameter p) { param = p; }

//    void loadProblems();
//    void writeProblems();

private:
    svm_parameter param;
    svm_problem prob;
    svm_model *model;
    QVector<QPointF> scale;
    QList<QPair<double, QVector<float> > > problems;

private slots:
    svm_model *buildModel(QList<QPair<double, QVector<float> > > problems, QVector<QPointF> scale);
    QVector<QPointF> calcScale(QList<QPair<double, QVector<float> > > _problems);
    float scaling(float value, QPointF maxmin);
};

#endif // SVMCLASSIFIER_H
