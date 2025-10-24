
#ifndef ActiveAcousticSensor_H

#define ActiveAcousticSensor_H
#define AIF
#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include <QDataStream>
#ifdef AIF
#include <QtMultimedia>
#include <fftw3.h>
#endif
#include <QInputDialog>
#include <QSlider>
#include <QThread>
#include <math.h>
#include <QTimer>


// AIF版とシリアル(USB/Bluetooth)版の基底クラス
class ActiveAcousticSensor : public QObject
{
    Q_OBJECT
public:
    ActiveAcousticSensor(QObject *parent = 0) : QObject(parent) {}
    ~ActiveAcousticSensor() {}

signals:
    void senseDataChanged(QVector<float> data);

public slots:
    virtual QString start() = 0;
    virtual void stop() = 0;
    virtual void setVolume(int value) = 0;
    virtual void calib() = 0;
    QVector<float> getData() { return data; }

protected:
    QVector<float> data;

};


/*====================================================================================================================================================================================================================================================================================*/
// AIF版ならば
#ifdef AIF
// スイープジェネレートをソフト側で行う
class SweepGenerator : public QIODevice
{
    Q_OBJECT
public:
    SweepGenerator(const QAudioFormat &format, int min_Hz, int max_Hz, int duration_ms, QObject *parent);

    void start();
    void stop();
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    qint64 bytesAvailable() const;

public:
    qint64 max_sample;

private:
    void generateData(const QAudioFormat &format, int min_Hz, int max_Hz, int duration_ms);

private:
    QAudioFormat m_format;
    qint64 m_pos;
    QByteArray m_buffer;
};

// AIF版 (AAS継承)
class AIFActiveAcousticSensor : public ActiveAcousticSensor
{
    Q_OBJECT
public:
    AIFActiveAcousticSensor(QString inputDeviceName, QString outputDeviceName, QObject *parent = 0);
    ~AIFActiveAcousticSensor();

public slots:
    QString start();
    void stop();
    void setVolume(int value);
    void calib() {}

private slots:
    void readData();
    void updateData() { emit senseDataChanged(data); }
private:
    // 周波数からインデックスに変換
    inline int hz2idx(int hz)
    {
        return (frame_width/2 * (hz/(float)(format.sampleRate()/2)));
    }

private:
    QTimer t;
    QVector<float> senseBuffer, anotherBuffer;
    int frame_width;
    QAudioFormat format;
    QAudioInput *input;
    QAudioOutput *output;
    QIODevice *inputBuffer;
    SweepGenerator *sweepGenerator;
    // 周波数レンジがハードコーディングされていたので変数を追加
    int _min_Hz;
    int _max_Hz;
};
#endif


/*====================================================================================================================================================================================================================================================================================*/
// シリアル(USB/Bluetooth)版 (AAS継承)
// 本AIF版では未使用
class SerialActiveAcousticSensor : public ActiveAcousticSensor
{
    Q_OBJECT
public:
    explicit SerialActiveAcousticSensor(QString deviceName, QObject *parent = 0);
    ~SerialActiveAcousticSensor();

signals:
    void senseDataChanged(QVector<float> data);

public slots:
    QString start();
    void stop();
    void setVolume(int value);
    void calib();

private slots:
    // serialがreadyReadシグナルを発行したら、データ読みだし処理と特徴ベクトル生成を行い、
    // mainWindowに処理を引き継ぐ
    void readData();
    // エラーをデバッグ標準出力に出す
    void reportError(QSerialPort::SerialPortError err) {
        qDebug() << err << serial.errorString();
    }

private:
    int vol;
    QVector<float> previousVector;
    QByteArray buf; // バッファ
    // シリアル通信用クラス(QIODeviceの派生クラス)
    QSerialPort serial; // シリアルポート
};


#endif // ActiveAcousticSensor_H
