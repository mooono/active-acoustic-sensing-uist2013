#include "plotter.h"
#include <QDebug>

Plotter::Plotter() : QGLWidget(QGLFormat(QGL::SampleBuffers))
{
    circleMode = false;
    setAutoFillBackground(false);
    setMinimumSize(150,50);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    base = 0;
    bi = 1;
    reg = 0;

    color_templates.append(QColor(67,130,185).lighter());
    color_templates.append(QColor(199,74,73).lighter());
    color_templates.append(QColor(161,203,98).lighter());
    color_templates.append(QColor(136,99,161).lighter());
    color_templates.append(QColor(3,169,191).lighter());
    color_templates.append(QColor(239,144,48).lighter());
    color_templates.append(QColor(155,180,216).lighter());
    color_templates.append(QColor(227,156,256).lighter());
    color_templates.append(QColor(188,214,159).lighter());
    setColor(Qt::gray);
}
