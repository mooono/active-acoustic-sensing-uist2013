#include "trainlabel.h"

#define BAR_SPEC 10

int TrainLabel::buffer_size = 20;
bool PredictionLabel::isReg = false;
float TrainLabel::threshold = 3;

TrainLabel::TrainLabel(QString _name, QColor _color, ActiveAcousticSensor *_aas, QWidget *parent) :
    QWidget(parent)
  , color(_color)
  , mode(MANUAL)
  , mouse_over_data(-1)
  , isDefault(false)
  , f(100)
  , aas(_aas)
  , trainCount(0)
{

    blinker.setInterval(30);
    connect(&blinker, SIGNAL(timeout()), SLOT(blinkTick()));
    connect(&blinker, SIGNAL(timeout()), SLOT(update()));

    trainer.setInterval(30);
    connect(&trainer, SIGNAL(timeout()), SLOT(trainTick()));
    connect(&trainer, SIGNAL(timeout()), SLOT(update()));

    plabel = new PredictionLabel(_name, _color);
    dlabel = new DefinitionLabel(_name, _color);
    connect(dlabel, SIGNAL(nameChanged(QString)), &name, SLOT(setText(QString)));
    connect(dlabel, SIGNAL(nameChanged(QString)), plabel, SLOT(setName(QString)));
    connect(dlabel, SIGNAL(deleted()), this, SIGNAL(deleted()));
    name.setText(_name);

    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    this->setFixedHeight(30);
    this->setMouseTracking(true);
}

TrainLabel::~TrainLabel()
{

}


QList<QVector<float> > TrainLabel::getTrainData()
{
    QList<QVector<float> > out;
    foreach(QList<QVector<float> > d, trainData)
    {
        out << d;
    }
    return out;
}


void TrainLabel::trainTick()
{
    QVector<float> d = aas->getData();
    float dif = diff(d, thresholdVector);

    qDebug() << dif << threshold;
    if(mode == AUTO)
    {
        if(dif > threshold)
        {
            updateTraining(d);
        }
        else
        {
            trainBuf.size() == buffer_size ? finishTraining() : suspendTraining();
        }
    }
    else
    {
        updateTraining(d);
    }
}

void TrainLabel::setTrain(TRAIN_MODE _mode, QVector<float> _thresholdVector)
{
    initTraining();
    mode = _mode;
    switch(mode)
    {
    case AUTO:
        if(!_thresholdVector.isEmpty()) thresholdVector = _thresholdVector;
        blinker.start();
        trainer.start();
        break;
    case FORCE:
        trainer.start();
        break;
    default:
        break;
    }

}

void TrainLabel::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    markRect = rect();
    markRect.setWidth(10);
    QColor markerColor = isDefault ? Qt::darkGray : color.lighter(110);
    if(blinker.isActive()) markerColor = markerColor.lighter(sin(M_PI * 2 * f / 50.)*20 + 100);
    p.fillRect(markRect, QBrush(markerColor));
    QWidget::paintEvent(event);
    int x = trainBuf.size()/(float)buffer_size * (width()-10);
    p.fillRect(QRectF(10, 0, x, height()), Qt::white);

    int left = width();
    for(int i = 0; i < trainData.count(); i++)
    {
        QRectF r = rect();
        r.setTop(height()*0.1);
        r.setHeight(height()*0.8);
        left = width()-(BAR_SPEC+3)*(i+1);
        r.setLeft(left);
        r.setWidth(BAR_SPEC);

        p.fillRect(r, mouse_over_data == i ? Qt::black : isDefault ? Qt::darkGray : QBrush(color.lighter(120)));
    }
    QRectF countRect = rect();
    countRect.setLeft(left - 15);
    p.setPen(QPen(Qt::darkGray, 4));
    p.drawText(countRect, Qt::AlignVCenter, QString::number(trainData.count()));


    QFont f("Helvetica", 18);
    f.setUnderline(isDefault);
    p.setFont(f);
    p.setPen(Qt::black);
    textRect = rect();
    textRect.setLeft(20);
    p.drawText(textRect, Qt::AlignVCenter, name.text());
}

bool TrainLabel::updateTraining(QVector<float> data)
{
    if(trainBuf.count() == buffer_size) return false;

    trainBuf.append(data);
    if(trainBuf.count() == buffer_size)
    {
        trainData.append(trainBuf);
        if(mode == FORCE) finishTraining();
    }
    return true;
}

void TrainLabel::initTraining()
{
    trainer.stop();
    blinker.stop();
    trainBuf.clear();
    update();
}

void TrainLabel::finishTraining()
{
    trainer.stop();
    blinker.stop();
    trainBuf.clear();
    trainCount++;
    update();
    emit trainFinished();
}

void TrainLabel::suspendTraining()
{
    if(mode == MANUAL) trainer.stop();
    trainBuf.clear();
    update();
}


void TrainLabel::mouseMoveEvent(QMouseEvent *event)
{
    mouse_over_data = -1;
    for(int i = 0; i < trainData.count(); i++)
    {
        QRectF r = rect();
        r.setTop(height()*0.1);
        r.setHeight(height()*0.8);
        r.setLeft(width()-(BAR_SPEC+3)*(i+1));
        r.setWidth(BAR_SPEC);

        if(r.contains(event->pos()))
        {
            mouse_over_data = i;
        }
    }
    update();
}


void TrainLabel::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    if(mode == MANUAL)
    {
        if(markRect.contains(event->pos()))
        {
            emit defaultPressed();
        }
        else if(mouse_over_data == -1)
            trainer.start();
        else
            trainData.removeAt(mouse_over_data);
    }
}

void TrainLabel::manualStart()
{
    trainer.start();
}


void TrainLabel::manualStop()
{
    trainBuf.count() == buffer_size ? finishTraining() : suspendTraining();
}

void TrainLabel::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
    if(mode == MANUAL)
    {
        trainBuf.count() == buffer_size ? finishTraining() : suspendTraining();
    }
}
