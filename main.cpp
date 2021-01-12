#include <QApplication>
#include"mandelbrotwidget.h"


int main(int argc, char *argv[]){

    //在支持的平台上启用Qt中的高DPI缩放
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    //生成高dpi的像素图，可以大于要求的大小。
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc,argv);
    MandelbrotWidget widget;
    widget.show();

    return app.exec();

}
