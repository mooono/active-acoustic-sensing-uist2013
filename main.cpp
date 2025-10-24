#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // アプリケーションクラス(ランタイム)生成
    QApplication a(argc, argv);
    // mainwindow.hで定義のMainWindowクラスのインスタンスを生成
    MainWindow w;
    
    // メインウィンドウ表示前にAIF選択のコンフィグウィンドウを表示したいため、ここではw.show();は行わない
    
    // イベントループ開始
    return a.exec();
}
