#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QThread>
#include <QMutex> //提供线程间的访问序列化
#include <QSize> //用整数定义二维对象的大小
#include <QWaitCondition> //用于同步线程的条件变量

QT_BEGIN_HEADER //命名空间
class QImage;
QT_END_NAMESPACE

//QThread是 Qt 中所有线程控制的基础。每个QThread 实例表示并控制一个线程。

class RenderThread : public QThread
{
    Q_OBJECT
public:
    RenderThread(QObject *parent = nullptr);
    ~RenderThread();
    //渲染大小
    void render(double centerX,double centerY,double scaleFactor,QSize resultSize,double devicePixelRatio);

signals:
    //当线程完成渲染一个图像时发出这信号。渲染图片成功
    void renderedImage(const QImage &image, double scaleFactor);

protected:
    //线程的起点。在调用start()之后，新创建的线程调用这个函数。重新实现这个函数以便高级线程管理。从这个方法返回将结束线程的执行。
    void run()override;

private:
    //rgb颜色波长。返回颜色
    static uint rgbFromWaveLength(double wave);

    QMutex mutex; //互斥锁
    QWaitCondition condition; //用于同步线程的条件变量
    double centerX;
    double centerY;
    double scaleFactor; //比例
    double devicePixelRatio; //图案像素配给量
    QSize resultSize; //实际尺寸
    bool restart = false; //重新启动
    bool abort = false; //中断
    enum {ColormapSize = 512}; //颜色表的大小
    uint colormap[ColormapSize]; //无符号整数


};

#endif // RENDERTHREAD_H
