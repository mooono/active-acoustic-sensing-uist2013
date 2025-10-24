#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include <QMessageBox>
#include "trainlabel.h"
#include "activeacousticsensor.h"
#include "plotter.h"
#include "svmclassifier.h"
#include <QSerialPortInfo>
#include <QKeyEvent>

#define MAXLABEL 20 // ラベル作成数上限。デフォルトは7
#define ON_MARK QPixmap(":/img/img/on.png")
#define OFF_MARK QPixmap(":/img/img/off.png")


// 起動時のAIF設定ウィジェット
class ConfigWidget : public QDialog
{
    Q_OBJECT
public:
    ConfigWidget(QWidget *parent = 0);

    QString getInputName() { return audioInputs.currentText(); }
    QString getOutputName() { return audioOutputs.currentText(); }
private:
    void setupUI();
    QComboBox audioInputs, audioOutputs;
    QPushButton okButton;
};


// 閾値スライダ
class ComparableSlider : public QSlider
{
    Q_OBJECT
public:
    ComparableSlider(Qt::Orientation o = Qt::Vertical, QWidget *parent = NULL)
        : QSlider(o, parent)
        , cmp(0)
    {

    }

public slots:
    void setCompareValue(int value)
    {
        cmp = value;
        update();
    }
    
protected:
    // 画面再描画イベントハンドラであるpaintEvent(cocoaのdrawRectと同様)をオーバーライド
    // QSliderに閾値バーを重ね書きするカスタム描画を行う
    void paintEvent(QPaintEvent *ev)
    {
        QPainter p(this);
        QRect cmpRect;
        QRect acceptRect;
        QRect rejectRect;
        if(orientation() == Qt::Vertical)
        {
            int t = (1 - cmp / (float)maximum()) * height();
            int h = height() - t;
            cmpRect = QRect(0, t, width(), h);
            t = (1 - this->value() / (float)maximum()) * height();
            h = height() - t;
            rejectRect = QRect(width()/2, t, width()/2, h);
            acceptRect = QRect(width()/2, 0, width()/2, t);
        }
        else
        {

        }
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(Qt::gray).lighter(130));
        p.drawRect(cmpRect);
        p.setBrush(QColor(Qt::red).lighter());
        // p.drawRect(rejectRect);
        p.setBrush(QColor(Qt::green).lighter());
        // p.drawRect(acceptRect);
        QSlider::paintEvent(ev);
    }

private:
    int cmp;

};


// メインウィンドウ
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    enum TAB {
        LABEL,
        TRAIN,
        PREDICT
    };

private:
    // メンバ変数
    QVBoxLayout *definitionLay, *trainingLay, *predictionLay;
    QTabWidget tab;
    Plotter plotter;
    ComparableSlider volumeSlider;
    QPushButton createLabelButton, manualButton, autoButton;
    QButtonGroup trainingMethod;
    QSpinBox trainTimes;
    QLineEdit labelEdit;
    QVector<float> thresholdVector;
    ConfigWidget conf;
    
    QList<TrainLabel *> labelList;
    QList<QColor> color_templates;
    QList<QColor> colorTrash;
    QInputDialog inputMethod;
    
    ActiveAcousticSensor *aas;
    SVMClassifier svm;
    TrainLabel *defaultLabel;

private slots:
    // アクションメソッド
    void createLabelButtonPushed();
    void addNewLabel(QString name);
    void senseDataChanged(QVector<float> senseData);
    void tabChanged(int tab);
    void labelDeleted();
    void trainFinshed();
    void defaultChanged();
    void switchAutoMode(bool b);
    void threshChanged(int v);

protected:
    // trainタブに居るときに数字キーを押すことで、マニュアルモードでラベルを押し続けるのと同じ動作(学習)を行う
    // 押下時
    void keyPressEvent(QKeyEvent *ev)
    {
        int key = ev->key() - Qt::Key_0 - 1;
        if(tab.currentIndex() == TRAIN && key >= 0 && key < labelList.count())
        {
            labelList.at(key)->manualStart();
        }
        QMainWindow::keyPressEvent(ev);
    }
    // リリース時
    void keyReleaseEvent(QKeyEvent *ev)
    {

        int key = ev->key() - Qt::Key_0 - 1;
        if(tab.currentIndex() == TRAIN && key >= 0 && key < labelList.count())
        {
            labelList.at(key)->manualStop();
        }
        QMainWindow::keyReleaseEvent(ev);
    }

// ユーティリティ
private:
    void sleep(int ms)
    {
        QElapsedTimer es;
        es.start();
        while(es.elapsed() < ms) qApp->processEvents();
    }
    
    QVector<float> calcThresholdVector(QList<QVector<float> > data)
    {
        int datasize = data.first().count();
        QVector<float> thresholdVector;
        thresholdVector.resize(datasize);
        for(int i = 0; i < data.count(); i++)
        {
            for(int j = 0; j < datasize; j++)
            {
                thresholdVector[j] += data[i][j] / (float)data.count();
            }
        }

        return thresholdVector;
    }
};

#endif // MAINWINDOW_H
