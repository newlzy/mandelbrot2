#include "renderthread.h"
#include <QImage>
#include <cmath> //数学函数

RenderThread::RenderThread(QObject *parent):QThread(parent)
{
    //确定颜色
    for(int i = 0; i < ColormapSize; ++i)
        colormap[i] = rgbFromWaveLength(380.0 + (i * 400.0 / ColormapSize));
}

RenderThread::~RenderThread()
{
    mutex.lock(); //互斥锁
    abort = true;
    //唤醒一个正在等等条件的线程。被唤醒的线程依赖于操作系统的调度策略，无法控制或预测。
    condition.wakeOne();
    mutex.unlock(); //解锁互斥
    wait(); //阻塞线程，直到满足以下任一条件
}

//渲染大小
void RenderThread::render(double centerX, double centerY, double scaleFactor, QSize resultSize, double devicePixelRatio)
{
    //在复杂的函数和语句或异常处理代码中锁定和解锁QMutex是容易出错且难于调试的。在这种情况下，可以使用QMutexLocker来确保互斥锁的状态总是良好定义的。应该在需要锁定QMutex的函数中创建QMutexLocker。在创建QMutexLocker时锁定互斥锁。您可以使用unlock()和relock()来解锁和重新锁定互斥对象。如果被锁定，那么当QMutexLocker被销毁时，互斥锁将被解锁。
    QMutexLocker locker(&mutex);

    this->centerX = centerX;
    this->centerY = centerY;
    this->scaleFactor = scaleFactor;
    this->devicePixelRatio = devicePixelRatio;
    this->resultSize = resultSize;

    if(!isRunning()){ //线程正在运行返回true
        start(LowPriority); //执行线程。优先级为低。这里会去执行run线程的起点
    }else{
        restart = true; //线程在运行
        condition.wakeOne(); //唤醒一个正在等待条件的进程。
    }

}



//线程的起点。绘制第一个分形图像。
void RenderThread::run()
{
    //循环
    forever{
        //锁的互斥锁。如果另一个线程锁定了互斥锁，那么这个调用将会阻塞，直到该线程解锁了互斥锁。
        mutex.lock();
        //图案像素配给量
        const double devicePixelRatio = this->devicePixelRatio;
        //实际尺寸
        const QSize resultSize = this->resultSize * devicePixelRatio;
        const double requestedScaleFactor = this->scaleFactor;
        //比例
        const double scaleFactor = requestedScaleFactor / devicePixelRatio;
        const double centerX = this->centerX;
        const double centerY = this->centerY;
        //解锁互斥量。尝试在另一个线程上解锁互斥锁会导致错误。解锁未锁定的互斥锁会导致未定义的行为。
        mutex.unlock(); //解锁

        int halfWidth = resultSize.width() / 2; //半宽
        int halfHeight = resultSize.height() / 2; //半高
        QImage image(resultSize,QImage::Format_ARGB32);
        //设置图像的设备像素比例。这是图像像素和设备无关像素之间的比率。
        image.setDevicePixelRatio(devicePixelRatio);

        const int NumPasses = 8;
        int pass = 0;
        while (pass < NumPasses) {
            const int MaxIterations = (1 << (2 * pass + 6)) + 32;
            const int Limit = 4;
            bool allBlack = true;

            for (int y = -halfHeight; y < halfHeight; ++y) {
                if (restart)
                    break;
                if (abort)
                    return;

                auto scanLine =
                        reinterpret_cast<uint *>(image.scanLine(y + halfHeight));
                const double ay = centerY + (y * scaleFactor);

                for (int x = -halfWidth; x < halfWidth; ++x) {
                    const double ax = centerX + (x * scaleFactor);
                    double a1 = ax;
                    double b1 = ay;
                    int numIterations = 0;

                    do {
                        ++numIterations;
                        const double a2 = (a1 * a1) - (b1 * b1) + ax;
                        const double b2 = (2 * a1 * b1) + ay;
                        if ((a2 * a2) + (b2 * b2) > Limit)
                            break;

                        ++numIterations;
                        a1 = (a2 * a2) - (b2 * b2) + ax;
                        b1 = (2 * a2 * b2) + ay;
                        if ((a1 * a1) + (b1 * b1) > Limit)
                            break;
                    } while (numIterations < MaxIterations);

                    if (numIterations < MaxIterations) {
                        *scanLine++ = colormap[numIterations % ColormapSize];
                        allBlack = false;
                    } else {
                        *scanLine++ = qRgb(0, 0, 0);
                    }
                }
            }

            if (allBlack && pass == 0) {
                pass = 4;
            } else {
                if (!restart)
                    emit renderedImage(image, requestedScaleFactor);
//! [5] //! [6]
                ++pass;
            }
//! [6] //! [7]
        }
//! [7]


        //关闭程序时把线程锁释放
        mutex.lock(); //互斥锁
        if(!restart){ //重启
            //wait释放互斥，并等待条件。重点是等待。
            condition.wait(&mutex);
        }
        restart = false;
        mutex.unlock(); //解锁互斥
    }
}

//rgb颜色波长。返回颜色
uint RenderThread::rgbFromWaveLength(double wave)
{
    double r = 0;
    double g = 0;
    double b = 0;

    if (wave >= 380.0 && wave <= 440.0) {
        r = -1.0 * (wave - 440.0) / (440.0 - 380.0);
        b = 1.0;
    } else if (wave >= 440.0 && wave <= 490.0) {
        g = (wave - 440.0) / (490.0 - 440.0);
        b = 1.0;
    } else if (wave >= 490.0 && wave <= 510.0) {
        g = 1.0;
        b = -1.0 * (wave - 510.0) / (510.0 - 490.0);
    } else if (wave >= 510.0 && wave <= 580.0) {
        r = (wave - 510.0) / (580.0 - 510.0);
        g = 1.0;
    } else if (wave >= 580.0 && wave <= 645.0) {
        r = 1.0;
        g = -1.0 * (wave - 645.0) / (645.0 - 580.0);
    } else if (wave >= 645.0 && wave <= 780.0) {
        r = 1.0;
    }

    double s = 1.0;
    if (wave > 700.0)
        s = 0.3 + 0.7 * (780.0 - wave) / (780.0 - 700.0);
    else if (wave <  420.0)
        s = 0.3 + 0.7 * (wave - 380.0) / (420.0 - 380.0);

    r = std::pow(r * s, 0.8);
    g = std::pow(g * s, 0.8);
    b = std::pow(b * s, 0.8);
    //返回颜色
    return qRgb(int(r * 255), int(g * 255), int(b * 255));
}
