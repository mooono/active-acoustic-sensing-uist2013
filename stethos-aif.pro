TEMPLATE = app
TARGET = stethos-aif
QT += core gui multimedia serialport opengl network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

LIBS += -L/usr/local/opt/fftw/lib -L/usr/local/opt/libsvm/lib -lfftw3f -lsvm
INCLUDEPATH += /usr/local/opt/fftw/include /usr/local/opt/libsvm/include

SOURCES += main.cpp\
        mainwindow.cpp \
    svmclassifier.cpp \
    activeacousticsensor.cpp \
    trainlabel.cpp \
    plotter.cpp

HEADERS  += mainwindow.h \
    svmclassifier.h \
    activeacousticsensor.h \
    trainlabel.h \
    plotter.h

RESOURCES += \
    resource.qrc
