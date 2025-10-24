#ifndef TRAINLABEL_H
#define TRAINLABEL_H

#include <QtWidgets>
#include <math.h>
#include "activeacousticsensor.h"
#include "svmclassifier.h"



class DefinitionLabel : public QWidget
{
    Q_OBJECT
public:
    DefinitionLabel(QString _name, QColor _color, QWidget *parent = 0)
        : QWidget(parent)
        , color(_color)
    {
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        this->setFixedHeight(30);
        name.setText(_name);
        name.setFixedHeight(30);
        connect(&name, SIGNAL(textChanged(QString)), SIGNAL(nameChanged(QString)));
        connect(&name, SIGNAL(editingFinished()), &name, SLOT(hide()));
        QVBoxLayout *lay = new QVBoxLayout;
        lay->setMargin(0);
        lay->setSpacing(0);
        lay->addWidget(&name);
        this->setLayout(lay);
        name.hide();
    }

signals:
    void nameChanged(QString name);
    void deleted();

protected:
    void paintEvent(QPaintEvent *event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        //mark
        QRectF markRect = rect();
        markRect.setWidth(10);
        p.fillRect(markRect, QBrush(color.lighter(30 + 80)));

        //parent
        QWidget::paintEvent(event);

        //close button
        if(underMouse())
        {
            closeRect = rect();
            closeRect.setLeft(width()-30);
            p.drawImage(closeRect, QImage(":/img/img/close.png"));
        }

        //text
        p.setFont(QFont("Helvetica", 18));
        p.setPen(Qt::black);
        QRectF textRect = rect();
        textRect.setLeft(20);
        p.drawText(textRect, Qt::AlignVCenter, name.text());
    }

    void enterEvent(QEvent *) { update(); }
    void leaveEvent(QEvent *) { update(); }
    void mouseDoubleClickEvent(QMouseEvent *event) { name.show(); }
    void mousePressEvent(QMouseEvent *event)
    {
        QWidget::mousePressEvent(event);
        if(closeRect.contains(event->pos())) emit deleted();
    }

private:
    QRect closeRect;
    QColor color;
    QLineEdit name;

};



class PredictionLabel : public QWidget
{
    Q_OBJECT
public:
    PredictionLabel(QString _name, QColor _color, QWidget *parent = 0)
        : QWidget(parent)
        , name(_name)
        , color(_color)
        , prob(0)
        , reg(0)
        , result(false)
    {
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        this->setFixedHeight(30);
    }

protected:
    void paintEvent(QPaintEvent *event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        //back
        QColor c(229, 229, 229);
        if(result) c = Qt::white;
        p.fillRect(rect(), QBrush(c));

        //mark
        QRectF markRect = rect();
        markRect.setWidth(10);
        p.fillRect(markRect, QBrush(color.lighter(30 + 80)));

        //reg
        QRectF regRect = rect();
        regRect.setHeight(5);
        regRect.setBottom(height());
        regRect.setLeft(width()/2 * ( 2. - reg));
        if(result && isReg) p.fillRect(regRect, Qt::gray);

        //parent
        QWidget::paintEvent(event);

        //text
        p.setFont(QFont("Helvetica", 18));
        p.setPen(Qt::black);
        QRectF textRect = rect();
        textRect.setLeft(20);
        p.drawText(textRect, Qt::AlignVCenter, name);

        //probability
        p.setFont(QFont("Helvetica", 12));
        p.setPen(Qt::gray);
        QRectF probRect = textRect;
        probRect.setLeft(width()-50);
        QString t;
        t = t.sprintf("%.4f", prob);
        p.drawText(probRect, Qt::AlignVCenter, t);
    }
public:
    static bool isReg;
    static void toggleReg()
    {
        isReg = !isReg;
    }

public slots:
    void setProbability(float value)
    {
        prob = value;
        update();
    }
    void setResult(bool b)
    {
        result = b;
    }
    void setReg(float value)
    {
        reg = value;
    }

    void setName(QString s) { name = s; update(); }
private:
    float reg;
    QString name;
    QColor color;
    float prob;
    bool result;
};

class TrainLabel : public QWidget
{
    Q_OBJECT
public:
    explicit TrainLabel(QString _name, QColor _color, ActiveAcousticSensor *_aas, QWidget *parent = 0);
    ~TrainLabel();

    typedef enum {
        MANUAL,
        AUTO,
        FORCE
    } TRAIN_MODE;

    static int buffer_size;
    static float threshold;

signals:
    void deleted();
    void defaultPressed();
    void trainFinished();

public slots:
    void setDefault(bool b) { isDefault = b; update(); }
    void setTrain(TRAIN_MODE _mode = FORCE, QVector<float> _thresholdVector = QVector<float>());
    void resetTrainCount() { trainCount = 0; }
    int getTrainCount() { return trainCount; }
    QList<QVector<float> > getTrainData();
    QColor Color() { return color; }
    QString Name() { return name.text(); }
    DefinitionLabel *getDefinitionLabel() { return dlabel; }
    PredictionLabel *getPredictionLabel() { return plabel; }
    void manualStart();
    void manualStop();


private slots:
    void blinkTick() { f = (++f) % 50; }

    void trainTick();
    void initTraining();
    bool updateTraining(QVector<float> data);
    void finishTraining();
    void suspendTraining();

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void enterEvent(QEvent *) { update(); }
    void leaveEvent(QEvent *) { mouse_over_data = -1; update(); }

private:
    void paramSetting();

private:
    int trainCount;
    int f;
    DefinitionLabel *dlabel;
    PredictionLabel *plabel;
    QLineEdit name;
    QHBoxLayout *hlay;
    QRect textRect, markRect, closeRect;
    QColor color;
    QTimer blinker, trainer;
    TRAIN_MODE mode;
    bool isDefault;
    int mouse_over_data;

    QList<QList<QVector<float> > > trainData;
    QList<QVector<float> > trainBuf;
    QVector<float> thresholdVector;

    ActiveAcousticSensor *aas;

public:
    static float diff(QVector<float> a, QVector<float> b)
    {
        float ret = 0;
        if(a.size() != b.size()) return -1;
        for(int i = 0; i < a.size(); i++)
        {
            ret += qAbs(a[i] - b[i]);
        }
        return ret;
    }

};

#endif // TRAINLABEL_H
