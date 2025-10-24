#include "svmclassifier.h"

SVMClassifier::SVMClassifier(QObject *parent) :
    QObject(parent)
  , model(NULL)
{
    param.svm_type = C_SVC;
    param.kernel_type = RBF;
    param.degree = 32;
    param.gamma = 0.0078125;
    param.coef0 = 1;
    param.nu = 0.2;
    param.cache_size = 100;
    param.C = 32;
    param.eps = 1e-3;
    param.p = 0.1;
    param.shrinking = 1;
    param.probability = 1;
    param.nr_weight = 0;
    param.weight_label = NULL;
    param.weight = NULL;
}

//void SVMClassifier::loadProblems()
//{
//    QString loadfile = QFileDialog::getOpenFileName();
//    if(loadfile.isEmpty()) return;
//    QFile f(loadfile);
//    f.open(QIODevice::ReadOnly);

//    QMap<int, QList<QVector<float> > > _problems;

//    QTextStream t(&f);
//    while(!t.atEnd())
//    {
//        QString line = t.readLine();
//        QStringList ll = line.split(" ");
//        int y = ll.takeFirst().toInt();
//        QVector<float> x;
//        foreach(QString s, ll)
//        {
//            x.append(s.split(":").last().toFloat());
//        }
//        _problems[y].append(x);
//    }
//    train(_problems);
//}

//void SVMClassifier::writeProblems()
//{
//    QString savefile = QFileDialog::getSaveFileName();
//    if(savefile.isEmpty()) return;
//    QFile out(savefile);
//    out.open(QFile::WriteOnly | QFile::Text);
//    QTextStream stm(&out);

//    foreach(int label, problems.keys())
//    {
//        foreach(QVector<float> vector, problems[label])
//        {
//            stm << label;
//            for(int j = 0; j < vector.count(); j++)
//            {
//                stm << " " << j+1 << ":" << vector[j];
//            }
//            stm << endl;
//        }
//    }
//    out.close();
//}

float SVMClassifier::scaling(float value, QPointF maxmin)
{
    float ret;
    if(value == maxmin.y())
        ret = -1;
    else if(value == maxmin.x())
        ret = 1;
    else
        ret = -1 + 2 * (value-maxmin.y()) / (maxmin.x()-maxmin.y());
    return ret;
}

svm_model *SVMClassifier::buildModel(QList<QPair<double, QVector<float> > > _problems, QVector<QPointF> scale)
{
    if(_problems.isEmpty()) return NULL;

    int dimension = scale.size();
    svm_problem prob;
    prob.l = _problems.count();
    prob.x = new svm_node *[prob.l];
    prob.y = new double[prob.l];
    svm_node *x_space = new svm_node[(dimension+1) * prob.l];

    for(int i = 0; i < _problems.size(); i++)
    {
        for(int j = 0; j < dimension; j++)
        {
            x_space[(dimension+1) * i + j].index = j+1;
            x_space[(dimension+1) * i + j].value = scaling(_problems[i].second[j], scale[j]);
        }
        x_space[(dimension+1) * i + dimension].index = -1;
        prob.x[i] = &x_space[(dimension+1) * i];
        prob.y[i] = _problems[i].first;
    }

    return svm_train(&prob, &param);
}

double SVMClassifier::predict(QVector<float> data, double *probability)
{
    if(!model) return -1;

    int dimension = data.size();
    svm_node *svm_x = new svm_node[dimension+1];
    for(int i = 0; i < dimension; i++)
    {
        svm_x[i].index = i+1;
        svm_x[i].value = scaling(data[i], scale[i]);
    }
    svm_x[dimension].index = -1;


    double res;
    if(probability != NULL)
        res = svm_predict_probability(model, svm_x, probability);
    else
        res = svm_predict(model, svm_x);

    delete [] svm_x;
    return res;
}

QVector<QPointF> SVMClassifier::calcScale(QList<QPair<double, QVector<float> > > _problems)
{
    QVector<QPointF> scale;
    scale.resize(_problems.first().second.size());
    for(int i = 0; i < scale.size(); i++)
    {
        scale[i].setX(-100);
        scale[i].setY(100);
    }
    for(int i = 0 ; i < _problems.size(); i++)
    {
        for(int j = 0; j < scale.size(); j++)
        {
            float v = _problems[i].second[j];
            if(scale[j].x() < v) scale[j].setX(v);
            if(scale[j].y() > v) scale[j].setY(v);
        }
    }

    return scale;
}


void SVMClassifier::train(QList<QPair<double, QVector<float> > > _problems)
{
    if(_problems.isEmpty()) return;
    if(model != NULL) svm_free_model_content(model);
    problems = _problems;
    scale = calcScale(problems);
    model = buildModel(problems, scale);
}

