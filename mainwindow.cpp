#include "mainwindow.h"

// 起動時のAIF設定ウィジェット
ConfigWidget::ConfigWidget(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}
void ConfigWidget::setupUI()
{
    foreach(QAudioDeviceInfo info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
    {
        audioInputs.addItem(info.deviceName());
    }
    foreach(QAudioDeviceInfo info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
    {
        audioOutputs.addItem(info.deviceName());
    }

    QVBoxLayout *vlay = new QVBoxLayout;
    vlay->addWidget(new QLabel("Audio Input Device:"));
    vlay->addWidget(&audioInputs);
    vlay->addWidget(new QLabel("Audio Output Device:"));
    vlay->addWidget(&audioOutputs);
    okButton.setText("OK");
    connect(&okButton, SIGNAL(clicked()), SLOT(accept()));
    vlay->addWidget(&okButton);

    this->setLayout(vlay);
}

/*====================================================================================================================================================================================================================================================================================*/
// メインウィンドウ
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , defaultLabel(NULL)
{
    connect(&autoButton, SIGNAL(toggled(bool)), SLOT(switchAutoMode(bool)));

    //color templates
    color_templates.append(QColor(67,130,185).lighter());
    color_templates.append(QColor(199,74,73).lighter());
    color_templates.append(QColor(161,203,98).lighter(130));
    color_templates.append(QColor(136,99,161).lighter());
    color_templates.append(QColor(3,169,191).lighter());
    color_templates.append(QColor(239,144,48).lighter());
    color_templates.append(QColor(155,180,216).lighter());
    color_templates.append(QColor(227,156,256).lighter());
    color_templates.append(QColor(188,214,159).lighter());

    //definition widget
    labelEdit.setPlaceholderText(" label name");
    labelEdit.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    createLabelButton.setText("add");
    connect(&createLabelButton, SIGNAL(clicked()), SLOT(createLabelButtonPushed()));
    connect(&labelEdit, SIGNAL(returnPressed()), SLOT(createLabelButtonPushed()));
    //connect(&plotter, SIGNAL(wheeled(QWheelEvent*)), aas, SLOT(wheeled(QWheelEvent*)));
    labelEdit.setFocusPolicy(Qt::StrongFocus);
    QHBoxLayout *labelEditLay = new QHBoxLayout;
    labelEditLay->setSpacing(10);
    labelEditLay->addWidget(&labelEdit);
    labelEditLay->addWidget(&createLabelButton);
    definitionLay = new QVBoxLayout;
    definitionLay->setAlignment(Qt::AlignTop);
    definitionLay->addLayout(labelEditLay);
    definitionLay->addSpacing(3);
    QWidget *definitionWidget = new QWidget;
    definitionWidget->setLayout(definitionLay);

    //training widget
    QWidget *trainingWidget = new QWidget;
    trainingLay = new QVBoxLayout;
    manualButton.setText("Manual");
    manualButton.setCheckable(true);
    manualButton.setAutoExclusive(true);
    autoButton.setText("Auto");
    autoButton.setCheckable(true);
    autoButton.setAutoExclusive(true);
    trainingMethod.addButton(&manualButton);
    trainingMethod.addButton(&autoButton);
    trainingMethod.setExclusive(true);
    manualButton.setChecked(true);
    QHBoxLayout *trainingMethodLay = new QHBoxLayout;
    trainingMethodLay->setMargin(0);
    trainingMethodLay->setSpacing(0);
    trainingMethodLay->setAlignment(Qt::AlignLeft);
    trainingMethodLay->addWidget(&manualButton);
    trainingMethodLay->addWidget(&autoButton);
    trainingMethodLay->addSpacing(10);
    trainTimes.setRange(1, 10);
    trainTimes.setValue(3);
    trainingMethodLay->addWidget(&trainTimes);
    QLabel *timesLabel = new QLabel("times");
    trainingLay->setAlignment(Qt::AlignTop);
    trainingLay->addLayout(trainingMethodLay);
    trainingWidget->setLayout(trainingLay);
    connect(&autoButton, SIGNAL(toggled(bool)), &trainTimes, SLOT(setEnabled(bool)));
    connect(&manualButton, SIGNAL(toggled(bool)), timesLabel, SLOT(setEnabled(bool)));


    //prediction widget
    QWidget *predictionWidget = new QWidget;
    predictionLay = new QVBoxLayout;
    predictionLay->setAlignment(Qt::AlignTop);
    predictionWidget->setLayout(predictionLay);


    //plotter settings
    plotter.bi = 2;
    plotter.base = 1;
    plotter.setMinimumWidth(300);

    //tab settings
    tab.addTab(definitionWidget, "Label");
    tab.addTab(trainingWidget, "Train");
    tab.addTab(predictionWidget, "Predict");
    tab.setMinimumHeight(300);
    tab.setFixedWidth(250);

    connect(&tab, SIGNAL(currentChanged(int)), SLOT(tabChanged(int)));

    QHBoxLayout *mmainLay = new QHBoxLayout;
    mmainLay->addWidget(&tab);
    mmainLay->addWidget(&plotter);
    mmainLay->addWidget(&volumeSlider);

    QWidget *w = new QWidget;
    w->setLayout(mmainLay);

    this->setCentralWidget(w);
    
    // コンフィグウィジェット(起動時のAIF設定)を起動
    int ret = conf.exec();
    if(ret == 0)
    {
        QTimer::singleShot(0, this, SLOT(close()));
        return;
    }
    conf.getInputName();
    
    
    /////////////////////
    //  メインルーチン開始
    /////////////////////
    
    // ActiveAcousticSensorクラス(以降AAS)のインスタンスを生成
    aas = new AIFActiveAcousticSensor(conf.getInputName(), conf.getOutputName());
    // AASを開始。シリアル通信を行いそれを整理した特徴ベクトルの送信がこちらへ向けて行われる
    // 処理開始できたら文字列OKが返り、開始出来なかった場合はシリアルポートクラスのエラーが返る
    qDebug() << aas->start();
    // AASでは、シリアル通信で取得されたデータを特徴ベクトルとして纏めるごとに、senseDataChangedシグナルをemitするので、
    // それを当クラスにおいてsenseDataChangedスロットで回収して処理を行う
    connect(aas, SIGNAL(senseDataChanged(QVector<float>)), SLOT(senseDataChanged(QVector<float>)));
    
    // スライダの値変更シグナルを閾値変更スロットに繋ぐ
    connect(&volumeSlider, SIGNAL(valueChanged(int)), SLOT(threshChanged(int)));
    volumeSlider.setRange(0, 300);
    volumeSlider.setValue(30);
    
    // ウィンドウを表示
    this->show();
}
MainWindow::~MainWindow()
{
}


/*====================================================================================================================================================================================================================================================================================*/
// 閾値が変更されたとき
void MainWindow::threshChanged(int v)
{
    TrainLabel::threshold = v/10.f;
}

// オートモードに切り替え
void MainWindow::switchAutoMode(bool automode)
{
    if(automode)
    {
        if(defaultLabel == NULL)
        {
            manualButton.click();
            plotter.drawText("please select default label.", 2);
        }
        else
        {
            plotter.drawText("system will train default label");
            foreach(TrainLabel *t, labelList)
            {
                t->resetTrainCount();
            }
            defaultLabel->setTrain(TrainLabel::FORCE);
        }
    }
    else
    {
        plotter.clearText();
        foreach(TrainLabel *t, labelList)
        {
            t->setTrain(TrainLabel::MANUAL);
        }
    }
}

// タブ切り替えされたとき
void MainWindow::tabChanged(int num)
{
    switch(num)
    {
    case LABEL:
        plotter.setColor(Qt::gray);
        break;
    case TRAIN:
        if(labelList.isEmpty())
        {
            tab.setCurrentIndex(0);
            plotter.drawText("plaese define some labels.", 2);
            break;
        }
        plotter.setColor(Qt::gray);
        break;
    case PREDICT:
        //label check
        if(labelList.isEmpty())
        {
            tab.setCurrentIndex(0);
            plotter.drawText("plaese define some labels.", 2);
            return;
        }

        //train data check
        foreach(TrainLabel *t, labelList)
        {
            if(t->getTrainData().isEmpty())
            {
                tab.setCurrentIndex(1);
                plotter.drawText("plaese train all labels.", 2);
                return;
            }
        }

        //build svm model
        QList<QPair<double, QVector<float> > > problems;
        int id = 0;
        foreach(TrainLabel *t, labelList)
        {
            foreach(QVector<float> d, t->getTrainData())
            {
                problems.append(QPair<double, QVector<float> >((double)id, d));
            }
            id++;
        }
        svm.train(problems);

        break;
    }
}


/*====================================================================================================================================================================================================================================================================================*/
// シリアル通信で取得され纏められた特徴ベクトルが更新される度に実行
void MainWindow::senseDataChanged(QVector<float> senseData)
{
    // 波形描画クラスplotterに新しいフレームの特徴ベクトルを渡して描画を更新
    plotter.updateData(senseData);

    if(tab.currentIndex() == TRAIN && !thresholdVector.empty())
    {
        volumeSlider.setCompareValue(TrainLabel::diff(thresholdVector, senseData)*10);
    }

    if(tab.currentIndex() == PREDICT && !labelList.empty())
    {
        double *probability = new double[labelList.size()];
        // 推定を行う(尤度を返すようにしている際は尤度が返る)
        double res = svm.predict(senseData, probability);
        for(int i = 0; i < labelList.size(); i++)
        {
            bool isTrueLabel = (i == (int)res);
            if(isTrueLabel)
            {
                plotter.setColor(labelList[i]->Color());
            }
            labelList[i]->getPredictionLabel()->setResult(isTrueLabel);
            labelList[i]->getPredictionLabel()->setProbability(probability[i]);
        }
    }
}


/*====================================================================================================================================================================================================================================================================================*/
// 新しいラベル追加ボタンが押下されたとき
void MainWindow::createLabelButtonPushed()
{
    addNewLabel(labelEdit.text());
    labelEdit.clear();
}
// 新しいラベルを追加
void MainWindow::addNewLabel(QString name)
{
    if(name.isEmpty()) return;
    QStringList labelNameList;
    foreach(TrainLabel *l, labelList)
    {
        labelNameList.append(l->Name());
    }

    if(labelNameList.contains(name))
    {
        plotter.drawText("the label already exists.", 2);
        return;
    }

    if(labelList.count() == MAXLABEL)
    {
        plotter.drawText("too many labels.", 2);
        return;
    }

    QColor color = colorTrash.isEmpty() ? color_templates.at(labelList.count()) : colorTrash.takeFirst();
    TrainLabel *newlabel = new TrainLabel(name, color, aas);
    labelList.append(newlabel);
    definitionLay->addWidget(newlabel->getDefinitionLabel());
    trainingLay->addWidget(newlabel);
    predictionLay->addWidget(newlabel->getPredictionLabel());
    connect(newlabel, SIGNAL(defaultPressed()), SLOT(defaultChanged()));
    connect(newlabel, SIGNAL(deleted()), SLOT(labelDeleted()));
    connect(newlabel, SIGNAL(trainFinished()), SLOT(trainFinshed()));
}
// 訓練終了
void MainWindow::trainFinshed()
{
    if(autoButton.isChecked())
    {
        TrainLabel *label = dynamic_cast<TrainLabel *>(QObject::sender());
        QList<TrainLabel *> tlist = labelList;
        tlist.removeOne(defaultLabel);

        if(label == defaultLabel)
        {
            for(int i = 0; i < tlist.count(); i++)
            {
                if(tlist[i]->getTrainCount() < trainTimes.value())
                {
                    thresholdVector = defaultLabel->getTrainData().last();
                    tlist[i]->setTrain(TrainLabel::AUTO, thresholdVector);
                    plotter.drawText("please perform " +
                                     tlist[i]->Name() +
                                     " " +
                                     QString::number(trainTimes.value()-tlist[i]->getTrainCount()) +
                                     " times");
                    return;
                }
            }
            plotter.drawText("training finished.", 3);
            manualButton.click();
        }
        else
        {
            if(label->getTrainCount() < trainTimes.value())
            {
                label->setTrain(TrainLabel::AUTO);
                plotter.drawText("please perform " +
                                 label->Name() +
                                 " " +
                                 QString::number(trainTimes.value()-label->getTrainCount()) +
                                 " times");
            }
            else
            {
                sleep(500);
                defaultLabel->setTrain(TrainLabel::FORCE);
                plotter.drawText("train default label.");
            }
        }
    }
}
// デフォルトラベル変更
void MainWindow::defaultChanged()
{
    TrainLabel *label = dynamic_cast<TrainLabel *>(QObject::sender());
    if(defaultLabel == label)
    {
        label->setDefault(false);
        defaultLabel = NULL;
    }
    else
    {
        if(defaultLabel != NULL) defaultLabel->setDefault(false);
        label->setDefault(true);
        defaultLabel = label;
    }
}
// ラベル削除
void MainWindow::labelDeleted()
{
    TrainLabel *label = dynamic_cast<TrainLabel *>(QObject::sender());
    if(label == defaultLabel) defaultLabel = NULL;
    labelList.removeOne(label);
    definitionLay->removeWidget(label->getDefinitionLabel());
    predictionLay->removeWidget(label->getPredictionLabel());
    trainingLay->removeWidget(label);
    colorTrash.append(label->Color());
    label->getDefinitionLabel()->deleteLater();
    label->getPredictionLabel()->deleteLater();
    label->deleteLater();
}


