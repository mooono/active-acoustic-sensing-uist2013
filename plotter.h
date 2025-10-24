#ifndef PLOTTER_H
#define PLOTTER_H

#include <QGLWidget>
#include <QDebug>
#include <QWheelEvent>
#include <QTimer>
#define _USE_MATH_DEFINES
#include <math.h>

class Plotter : public QGLWidget
{
    Q_OBJECT
public:
    Plotter();

public:
    float base, bi;

signals:
    void wheeled(QWheelEvent *event);

protected:
    void wheelEvent(QWheelEvent *event)
    {
        bi += event->delta()/10.;
        emit wheeled(event);
    }

    void initializeGL()
    {
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA);

        qglClearColor(Qt::white);
    }
    void resizeGL(int w, int h)
    {
        glViewport(0, 0, w, h);

    }

    void drawForm()
    {
        bi = 2;
        base = 1;
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        qglClearColor(Qt::white);
        glClear(GL_COLOR_BUFFER_BIT );

        glLineWidth(3);
        glColor3f(color.redF(), color.greenF(), color.blueF());

        foreach(QVector<float> data, dataList)
        {
            if(data.isEmpty()) continue;
            glBegin(GL_TRIANGLE_STRIP);
            int end = data.count();

            glVertex2f(-1, -1);
            for(int i = 0; i < end; i++)
            {
                float x = (i - end/2)/(float)(end/2);
                float y = data.at(i) * bi - base;
                glVertex2f(x, y);
                glVertex2f(x, -1);

            }

            glEnd();
        }

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }

    void drawFormCircle()
    {
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        qglClearColor(Qt::white);
        glClear(GL_COLOR_BUFFER_BIT );

        glLineWidth(3);
        glColor3f(color.redF(), color.greenF(), color.blueF());

        foreach(QVector<float> data, dataList)
        {
            if(data.isEmpty()) continue;
            glBegin(GL_TRIANGLE_STRIP);
            int end = data.count();

            for(int i = 0; i < end; i++)
            {
                float th = M_PI * i / (float)(end);
                float r = data.at(i) * bi;
                float x =  r * sin(th);
                float y =  r * cos(th);;
                glVertex2f(x, y);
                glVertex2f(0, 0);
            }
            for(int i = 0; i < end; i++)
            {
                float th = M_PI + M_PI * i / (float)(end);
                float r = data.at(end-i-1) * bi;
                float x =  r * sin(th);
                float y =  r * cos(th);;
                glVertex2f(x, y);
                glVertex2f(0, 0);
            }
            float r = data.at(0.0) * bi;
            float x =  r * sin(0.0);
            float y =  r * cos(0.0);
            glVertex2f(x, y);
            glVertex2f(0, 0);

            glEnd();
        }
    }

    // 画面再描画イベントハンドラであるpaintEvent(cocoaのdrawRectと同様)をオーバーライド
    void paintEvent(QPaintEvent *event)
    {
        if(!dataList.isEmpty()) circleMode ? drawFormCircle() : drawForm();

        drawReg();
        QPainter painter(this);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::HighQualityAntialiasing);

        QColor b = Qt::darkGray;
        painter.setPen(b.darker());
        QFont f = QFont("Helvetica", 16);
        painter.setFont(f);
        if(!text.isEmpty()) painter.drawText(QRectF(20, 15, width(), 30), Qt::AlignVCenter, text);
        painter.end();
    }


public slots:
    void appendData(QVector<float> data)
    {
        dataList.append(data);
    }
    void clearData()
    {
        dataList.clear();
    }

    void setReg(float v)
    {
        reg = v;
        update();
    }

    void drawReg()
    {
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix(); {
            glColor3f(color.redF(), color.greenF(), color.blueF());
            glBegin(GL_POLYGON); {
                float y = (reg-0.5) * 2;
                float w = 0.4;
                glVertex2f(-1.0, +0.8);
                glVertex2f(y, +0.8);
                glVertex2f(y, +0.8-w);
                glVertex2f(-1.0, +0.8-w);
            } glEnd();
            glMatrixMode(GL_MODELVIEW);
        } glPopMatrix();
    }

    void updateData(QVector<float> data)
    {
        clearData();
        appendData(data);
        update();
    }
    void drawText(QString _text, int seconds = -1)
    {
        text = _text;
        if(seconds != -1) QTimer::singleShot(seconds * 1000, this, SLOT(clearText()));
        update();
    }

    void clearText()
    {
        text = "";
    }

    void toggleCircleMode()
    {
        circleMode = !circleMode;
    }

    //void setColor(int index) { color = color_templates[index]; }
    void setColor(QColor _color) { color = _color.lighter(130); }
private:
    bool circleMode;
    float reg;
    QString text;
    QList<QVector<float> > dataList;
    QList<QColor> color_templates;
    QColor color;



};

#endif // PLOTTER_H
