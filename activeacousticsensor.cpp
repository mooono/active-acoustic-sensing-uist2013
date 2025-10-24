#include "activeacousticsensor.h"

/*====================================================================================================================================================================================================================================================================================*/
// Math Functions

// ローパスフィルタ
QVector<float> lowpass(QVector<float> in, int width = 5)
{
    QVector<float> out;
    for(int i = 0; i < in.size(); i++)
    {
        float mean = 0;
        int count = 0;
        for(int j = qMax(i-width, 0); j < qMin(i+width, in.size()); j++)
        {
            if(in.value(j) != 0 )
            {
                mean += in[j];
                count++;
            }
        }
        mean /= (float)count;
        out.append(mean);
    }
    return out;
}

// データ(パワースペクトル)の次元を削減(ダウンサンプリング)
// stepで指定した間隔ごとにデータを取得して詰め直す
QVector<float> reduce(QVector<float> in, int step, int offset = 0)
{
    QVector<float> out;
    for(int i = 0; i < in.size(); i++)
    {
        if((i+offset) % step == 0 )
            out.append(in.at(i));
    }
    return out;
}

// ハミング窓
QVector<float> hamming(QVector<float> in)
{
    QVector<float> out;
    for(int i = 0; i < in.size(); i++)
    {
        double ham = 0.54 - 0.46 * cos(2.*M_PI*i/(double)in.size());
        out.append(ham*in.at(i));
    }
    return out;
}

/*====================================================================================================================================================================================================================================================================================*/
#ifdef AIF
// パワースペクトルを求める
int maGetPowerSpectol2D( fftwf_complex *in, float *out, int cols, int rows )
{
    int i,j;
    int idx; // index of data
    //double max, min, scale; // max/min of powerspectol

    if( in==NULL || out==NULL ) return false;
    if( rows<0 || cols<0 )      return false;

    for( j=0; j<rows; j++ ){
        for( i=0; i<cols; i++ ){
            idx = j*cols + i;
            out[idx] = log10(1 + sqrt(pow(in[idx][0],2) + pow(in[idx][1],2)) );
        }
    }

    return true;
}

// FFT
QVector<float> fft(QVector<float> in, bool ma = true)
{

    int N = in.size();
    float *inc = new float[N];
    memcpy(inc, in.data(), sizeof(float)*N);

    size_t mem_size = sizeof(fftwf_complex)*N;
    fftwf_complex *out = (fftwf_complex*)fftwf_malloc( mem_size );
    fftwf_plan p = fftwf_plan_dft_r2c_1d(N, inc, out, FFTW_ESTIMATE);
    fftwf_execute(p);
    delete [] inc;
    QVector<float> o(N/2);
    if(ma)
        maGetPowerSpectol2D(out, o.data(), 1, N/2);
    else
    {
        for(int i = 0; i < o.size(); i++)
        {
            o[i] = sqrt(pow(out[i][0],2) + pow(out[i][1],2));
        }
    }
    fftwf_destroy_plan(p);
    fftwf_free(out);
    return o;
}

/*====================================================================================================================================================================================================================================================================================*/
// スイープジェネレータ

/**
 * @brief SweepGenerator::SweepGenerator
 * @param format
 * @param min_Hz
 * @param max_Hz
 * @param duration_ms
 * @param parent
 */

SweepGenerator::SweepGenerator(const QAudioFormat &format, int min_Hz, int max_Hz, int duration_ms, QObject *parent = NULL)
    : QIODevice(parent) // 基底クラスのコンストラクタを明示的に呼んでおく
    , m_format(format) // メンバの初期化(通常はconstをつけたメンバなど関数内で初期化できないものを初期化するための書き方だが、今回はconst無しだった)
    , m_pos(0) // 同上
{
    generateData(format, min_Hz, max_Hz, duration_ms);
}

void SweepGenerator::start()
{
    open(QIODevice::ReadOnly);
}

void SweepGenerator::stop()
{
    m_pos = 0;
    close();
}

void SweepGenerator::generateData(const QAudioFormat &format, int min_Hz, int max_Hz, int duration_ms)
{
    max_sample = format.sampleRate() * duration_ms / 1000.;
    float df = (float)(max_Hz-min_Hz)/(float)max_sample;
    float noise_filter = 1;
    double fi = 0;

    m_buffer.clear();
    m_buffer.resize(max_sample * format.bytesPerFrame());
    unsigned char *ptr = reinterpret_cast<unsigned char *>(m_buffer.data());

    for(qint64 p = 0; p < max_sample; p++)
    {
        double f = min_Hz + df * p;

        if(p < max_sample/10)
            noise_filter = (float)p/(max_sample/10.);
        else if((max_sample-p) < max_sample/10)
            noise_filter = (float)(max_sample-p)/(max_sample/10.);
        else
            noise_filter = 1;
        qint16 value = static_cast<qint16>(qSin(fi)* noise_filter * (SHRT_MAX-1));
        for(int j = 0; j < format.channelCount(); j++)
        {
            qToLittleEndian<qint16>(value, ptr);
            ptr += 2;
        }
        fi += 2 * M_PI * f * (1./(double)format.sampleRate());
    }
}

qint64 SweepGenerator::readData(char *data, qint64 maxlen)
{
    qint64 total = 0;
    while (maxlen - total > 0) {
        const qint64 chunk = qMin((m_buffer.size() - m_pos), maxlen - total);
        memcpy(data + total, m_buffer.constData() + m_pos, chunk);
        m_pos = (m_pos + chunk) % m_buffer.size();
        total += chunk;
    }
    return total;
}

qint64 SweepGenerator::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 SweepGenerator::bytesAvailable() const
{
    return m_buffer.size() + QIODevice::bytesAvailable();
}

/*====================================================================================================================================================================================================================================================================================*/
// メイン機能

// AIF版AASコンストラクタ
// mainWindowから呼び出されて生成
AIFActiveAcousticSensor::AIFActiveAcousticSensor(QString inputDeviceName, QString outputDeviceName, QObject *parent)
    : ActiveAcousticSensor(parent)
    , frame_width(3840) // 3840
{
    senseBuffer.resize(frame_width);

    // サンプリングレート
    format.setSampleRate(96000);
    format.setCodec("audio/pcm");
    format.setSampleType(QAudioFormat::SignedInt);

    // IN/OUTオーディオデバイスを設定
    QAudioDeviceInfo inputDevice, outputDevice;
    foreach(QAudioDeviceInfo info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
    {
        if(info.deviceName().startsWith(inputDeviceName))
            inputDevice = info;
    }
    foreach(QAudioDeviceInfo info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
    {
        if(info.deviceName().startsWith(outputDeviceName))
            outputDevice = info;
    }

    // INのフォーマットを設定
    format = inputDevice.preferredFormat();
    format.setSampleSize(16);
    // INのインスタンスを生成
    input = new QAudioInput(inputDevice, format);
    // バッファサイズを設定
    input->setBufferSize(10000); // 10000
    // OUTのインスタンスを生成
    output = new QAudioOutput(outputDevice, format);

    // 周波数レンジを設定して、スイープジェネレータを生成
    _min_Hz = 20000;
    _max_Hz = 40000; // 82000より上で不可解な可聴ノイズ発生
    sweepGenerator = new SweepGenerator(format, _min_Hz, _max_Hz, 20);
    //sweepGenerator = new SweepGenerator(format, 20000, 40000, 20); // 20kHz~40kHz

    // タイマのタイムアウトイベントをデータ更新のトリガーに設定。タイマの設定はAIFActiveAcousticSensor::start()にて行われる。
    // 当該フレームの特徴ベクトルをdataとして添えて、データ更新シグナルupgateData(data)を発行。この実装はヘッダにて記述。
    // このシグナルは、シリアル版でMainTabにキャッチされているのと同様に、本AIF版ではmainWindowにてキャッチされる
    connect(&t, SIGNAL(timeout()), SLOT(updateData()));
}
AIFActiveAcousticSensor::~AIFActiveAcousticSensor()
{
    stop();
}


// mainWindowから叩かれてルーチンスタート
QString AIFActiveAcousticSensor::start()
{
    if(input->state() != QAudio::ActiveState)
    {
        inputBuffer = input->start();
        // 受信シグナルをreadyRead()を受けたらreadDataでデータ読み込みを行うように設定
        connect(inputBuffer, SIGNAL(readyRead()), SLOT(readData()));
        inputBuffer->open(QIODevice::ReadOnly);
        // senseDataChanged()シグナルを発行する更新間隔を設定
        t.start(33);
    }
    if(output->state() != QAudio::ActiveState)
    {
        sweepGenerator->start();
        output->start(sweepGenerator);
        t.start(33);
    }

    return "OK";
}
void AIFActiveAcousticSensor::stop()
{
    inputBuffer->close();
    inputBuffer->disconnect(SIGNAL(readyRead()));
    input->stop();

    sweepGenerator->close();
    output->stop();
}


// AIFからの入力バッファから一定時間内の音声データ(時間領域)を取得し、これをFFTしてパワースペクトルに変換後、データの加工を行う
void AIFActiveAcousticSensor::readData()
{
    QDataStream ds(inputBuffer->readAll());
    ds.setByteOrder(QDataStream::LittleEndian);

    short sample;
    int count = 0;
    while(!ds.atEnd())
    {
        ds >> sample;
        if(count % input->format().channelCount() == 1) // 0は1ch, 1は2ch
        {
            senseBuffer.takeFirst();
            senseBuffer.append(sample/(float)SHRT_MAX*10);
        }
        count++;
    }
    
    // 読み込んだデータ(この時点ではまだ時間領域)にハミング窓を掛けて不連続性を軽減し(http://www.logical-arts.jp/?p=124)、これをfftして周波数領域(パワースペクトル)に変換。
    // この中から必要な周波数レンジのデータのみをmidを用いて取り出す(コピー操作であることに注意)。第1引数は取り出し開始位置、第2引数はそこからの幅
    //QVector<float> rawData = fft(hamming(senseBuffer)).mid(hz2idx(20000), hz2idx(40000)-hz2idx(20000));
    QVector<float> rawData = fft(hamming(senseBuffer)).mid(hz2idx(_min_Hz), hz2idx(_max_Hz)-hz2idx(_min_Hz));
    
    // パワースペクトルの次元を1/2に削減してからローパスフィルタを掛け、これを加工済みデータとする
    data = lowpass(reduce(rawData,2));
}


// AIFでは音量調整はハード側で行うため、この関数は未使用
void AIFActiveAcousticSensor::setVolume(int value)
{

}
#endif


/*====================================================================================================================================================================================================================================================================================*/
// シリアル通信版AAS(USB/Bluetooth版)
// 本AIF版において以下のコードは使用されていないと考えられる

/**
 * @brief SerialActiveAcousticSensor::SerialActiveAcousticSensor
 * @param parent
 */

// シリアル通信版AASコンストラクタ
// メインウィンドウ起動時に選択されたデバイス名が渡される
SerialActiveAcousticSensor::SerialActiveAcousticSensor(QString deviceName, QObject *parent) :
    ActiveAcousticSensor(parent)
  , vol(128)
{
    // 受信シグナルreadyRead()を受けたらreadData()でデータ読み込み処理を行うように設定
    connect(&serial, SIGNAL(readyRead()), this, SLOT(readData()));
    // シリアル通信でエラーが発生したらデバッグ標準出力に流す(reportErrorの実装はヘッダにて)ように設定
    connect(&serial, SIGNAL(error(QSerialPort::SerialPortError)), SLOT(reportError(QSerialPort::SerialPortError)));
    // 得られる全てのシリアルポート情報から、指定した名前のポートのinfoをセット
    foreach(QSerialPortInfo info, QSerialPortInfo::availablePorts())
    {
        qDebug() << info.portName();
        if( info.portName().startsWith(deviceName))
            serial.setPort(info);
    }
}
SerialActiveAcousticSensor::~SerialActiveAcousticSensor()
{
    stop();
}


// シリアル通信を開始(MainWindowから叩かれる)
QString SerialActiveAcousticSensor::start()
{
    // ポートを開く
    if(serial.open(QSerialPort::ReadWrite))
    {
        // ボーレートを設定
        serial.setBaudRate(230400);
        qDebug() << serial.baudRate() << serial.isOpen() << serial.portName();
        // マイコン(mbed)にシリアルポート経由でスタートバイト(mbedのコードで定義している？)を送る
        char c = 0xFE;
        qDebug() << serial.write(&c, 1); // 第2引数は書き込むデータの最大サイズが1バイトであることを示す
        // 500ms経過するまでにbytesWritten()シグナル(ペイロードから1バイトがデバイスに書き込まれる度に発行)が
        // emitされたか否かをboolで返す
        bool ret = serial.waitForBytesWritten(500);
        // 500ms経過するまでにreadyRead()シグナルがemitされたか否かをboolで返す。先の結果とandをとる
        ret &= serial.waitForReadyRead(500);
        // 上記が両方とも真でなければ非対応デバイスであると返してここで抜ける
        if(!ret) return "This device is not supported.";
        
        // シングルショットでキャリブレーションを叩く。なぜ直接叩くのではなくタイマで0msを用いている?
        QTimer::singleShot(0, this, SLOT(calib()));
        
        // 以上が完了したらエラーなしフラグとしてOK文字列を返す
        return "OK";
    }
    else
    {
        return serial.errorString();
    }
}
// シリアル通信を終了(デストラクタで行う処理)
void SerialActiveAcousticSensor::stop()
{
    // 終了バイトを送る
    char c = 0xFF;
    serial.write(&c, 1);
    serial.waitForBytesWritten(10000);
    QThread::msleep(600);
    // シリアルポートを閉じてバッファを削除
    serial.close();
    buf.clear();
    qDebug() << "aa";
}


// serialがreadyReadシグナルを発行したら、データ読みだし処理と特徴ベクトル生成を行い、
// mainWindow(MainTab)のsenseDataChangedに処理を引き継ぐ
void SerialActiveAcousticSensor::readData()
{
    // シリアルポートからのデータをバイト配列に読み込む
    QByteArray a = serial.readAll(); //qDebug() << a.isEmpty();
    // QByteArray型のバッファ(buf)にアペンド
    buf.append(a);
    // バッファに詰め込まれているデータの中にチャンク終了バイトが含まれていれば
    if(buf.contains(0xFF)) // 0xFF(1111 1111)はハード側(mbed)のコードで定義しているチャンク終了バイトと思われる
    {
        // チャンク終了バイトごとにバイトストリームをチャンクに切り分け、これをリストに格納する
        QList<QByteArray> test = buf.split(0xFF);
        // 最初のチャンクを取得し、QDataStream型の変数streamに格納
        QDataStream stream(test.first()); // QDataStream stream = QDataStream(test.first());と同義
        // 生データ(law-data)用配列を用意
        QVector<float> ldata;
        // ストリームの終端に到達するまで繰り返す
        while(!stream.atEnd())
        {
            ushort val;
            stream >> val; // streamから1個読み込む
            // ushort型の最大値で割ってldataに追加。
            // mbedのADCにて、read()ではなくread_u16()を使用しているためであると考えられる。なぜu16を選択したのかは不明。
            // ※readは0~3.3Vに対して0~1を返す。read_u16は0~65535を返す。
            // USHRT_MAX=65535なので、これで割ることでread_u16の値を0~1に正規化できる
            ldata.append(val/(float)USHRT_MAX);
        }
        // 生データにローパスを掛けて、加工済みデータdata(QVector<float>型)とする。
        // dataはその瞬間(フレーム)の特徴ベクトルであり、スイープの段階分の次元を持つ
        data = lowpass(ldata,2);
        
        // 前のフレームの特徴ベクトルが今回の物と同じサイズならば…何をしている？
        if(previousVector.size() == data.size())
        {
            for(int i = 0; i < previousVector.size(); i++)
            {
                data[i] = (previousVector[i]+data[i])/2.0;
            }
        }
        
        // 当該フレームの特徴ベクトルをdataとして添えて、データ更新シグナルを発行。
        // mainWindowにてキャッチされる
        emit senseDataChanged(data);
        
        previousVector = data;
        buf = test.last();
    }
}


// 音量？閾値？をStethosハードにセット
void SerialActiveAcousticSensor::setVolume(int value)
{
    char cmd[2] = { 0x01, (char)(value) };
    if(serial.isWritable())
        serial.write(cmd, 2);
    serial.waitForBytesWritten(1000);
}
// キャリブレーション
void SerialActiveAcousticSensor::calib()
{
    // キャリブレーションバイトを送る
    char cmd = 0x00;
    if(serial.isWritable())
        serial.write(&cmd, 1);
    serial.waitForBytesWritten(1000);
}

