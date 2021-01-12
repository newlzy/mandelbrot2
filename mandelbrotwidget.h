#ifndef MANDELBROTWIDGET_H
#define MANDELBROTWIDGET_H

#include <QWidget>
#include "renderthread.h"
#include <QPixmap> //图像显示，可用做绘图设备

class MandelbrotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MandelbrotWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event)override;
    void resizeEvent(QResizeEvent *event) override;
    //事件处理程序，以接收小部件的按键事件
    void keyPressEvent(QKeyEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override; //滚轮事件
#endif
    void mousePressEvent(QMouseEvent *event) override; //鼠标左键按下获取位置
    void mouseMoveEvent(QMouseEvent *event) override; //鼠标移动后的位置
    void mouseReleaseEvent(QMouseEvent *event) override; //鼠标释放

private slots:
    //更新像素图
    void updatePixmap(const QImage &image ,double scaleFactor);
    void zoom(double zoomFactor); //缩放

private:
    void scroll(int deltaX, int deltaY); //左边按键
    RenderThread thread; //线程自定义类
    QPixmap pixmap;
    QPoint pixmapOffset; //像素图偏移量
    QPoint lastDragPos; //鼠标位置
    double centerX; //坐标x
    double centerY;
    double pixmapScale; //像素
    double curScale; //缩放
};

#endif // MANDELBROTWIDGET_H
