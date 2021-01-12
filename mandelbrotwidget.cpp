#include "mandelbrotwidget.h"
#include <math.h> //数学
#include <QPainter>
#include <QKeyEvent> //键事件
#pragma execution_character_set("utf-8")

const double DefaultCenterX = -0.637011; //坐标x
const double DefaultCenterY = -0.0395159; //y
const double DefaultScale = 0.00403897; //像素

const double ZoomInFactor = 0.8; //放大，加号
const double ZoomOutFactor = 1 / ZoomInFactor; //缩小，减号
const int ScrollStep = 20; //左右按键

MandelbrotWidget::MandelbrotWidget(QWidget *parent) : QWidget(parent),centerX(DefaultCenterX),centerY(DefaultCenterY),pixmapScale(DefaultScale),curScale(DefaultScale)
{
    //线程的起点会触发renderedImage信号。渲染图片后刷新像素图
    connect(&thread,&RenderThread::renderedImage,this,&MandelbrotWidget::updatePixmap);

    setWindowTitle(tr("多线程渲染"));

#if QT_CONFIG(cursor) //任意形状的鼠标光标
    setCursor(Qt::CrossCursor); //鼠标为十字形状
#endif

    resize(550,400);
}

void MandelbrotWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    //填充刷子。rect保存小部件的内部几何形状
    painter.fillRect(rect(),Qt::black);

    if(pixmap.isNull()){ //空像素时
       painter.setPen(Qt::white); //画笔白色
       //在提供的矩形内绘制给定的文本。AlignCenter两个维度都有中心。
       painter.drawText(rect(),Qt::AlignCenter,tr("正在渲染初始图像，请等待…"));
       return;
    }

    //比较两个值，相等返回true
    if(qFuzzyCompare(curScale,pixmapScale)){
        //以给定点为原点绘制给定像素图的矩形部分源。
        painter.drawPixmap(pixmapOffset,pixmap);
    }else{

        //显示图像的对象
        auto previewPixmap = qFuzzyCompare(pixmap.devicePixelRatioF(),qreal(1))? pixmap : pixmap.scaled(pixmap.size() / pixmap.devicePixelRatioF(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
        //重要代码。缩放
        double scaleFactor = pixmapScale / curScale;

        int newWidth = int(previewPixmap.width() * scaleFactor);
        int newHeight = int(previewPixmap.height() * scaleFactor);
        int newX = pixmapOffset.x() + (previewPixmap.width() - newWidth) / 2;
        int newY = pixmapOffset.y() + (previewPixmap.height() - newHeight) / 2;

        painter.save(); //保存状态
        painter.translate(newX,newY); //用向量来平移坐标
        //将坐标系统按参数来缩放
        painter.scale(scaleFactor,scaleFactor);

        //transform最底变换矩阵。inverted返回这个矩阵的反向副本。mapRect创建并返回一个QRect对象，该对象是给定矩形的副本，映射到由该矩阵定义的坐标系统中。adjusted返回一个新的矩形，分别在该矩形的现有坐标上添加dx1、dy1、dx2和dy2。
        QRect exposed = painter.transform().inverted().mapRect(rect()).adjusted(-1,-1,1,1);

        //将给定像素图的矩形部分源绘制到绘制设备中的给定目标中。
        //注意:如果像素图和矩形大小不一致，则像素图缩放以适应矩形。
        painter.drawPixmap(exposed,previewPixmap,exposed);
        painter.restore(); //恢复状态
    }

    QString text = tr("使用鼠标滚轮或‘+’和‘-’键缩放。按住鼠标左键滚动");
    QFontMetrics metrics = painter.fontMetrics(); //字体度量信息
    //把所有文字用相等的间距显示
    int textWidth = metrics.horizontalAdvance(text);

    painter.setPen(Qt::NoPen); //画笔
    painter.setBrush(QColor(0,0,0,127)); //画刷
    painter.drawRect((width() - textWidth) / 2 - 5, 0,textWidth + 10,metrics.lineSpacing()+5); //矩形
    painter.setPen(Qt::white);
    painter.drawText((width() - textWidth) / 2,metrics.leading() + metrics.ascent(),text); //绘制文字
}

void MandelbrotWidget::resizeEvent(QResizeEvent *event)
{
    //devicePixelRatioF以浮点数的形式返回设备像素比率
    thread.render(centerX,centerY,curScale,size(),devicePixelRatioF());
}

//
void MandelbrotWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Plus: //加号
        zoom(ZoomInFactor);
        break;
case Qt::Key_Minus: //减
        zoom(ZoomOutFactor);
        break;
    case Qt::Key_Left: //左边
        scroll(-ScrollStep,0);
        break;
    case Qt::Key_Right: //右
        scroll(+ScrollStep,0);
        break;
    case Qt::Key_Down: //向下
        scroll(0,-ScrollStep);
        break;
    case Qt::Key_Up: //上
        scroll(0,+ScrollStep);
        break;
    default:
        QWidget::keyPressEvent(event); //这行代码一定要
    }
}

//鼠标按下
void MandelbrotWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() ==Qt::LeftButton) //鼠标左键
        lastDragPos = event->pos(); //位置
}

//鼠标移动后的位置
void MandelbrotWidget::mouseMoveEvent(QMouseEvent *event)
{
    //buttons返回一个位域，用于检查可能伴随鼠标事件的鼠标按钮
    if(event->buttons() & Qt::LeftButton){
        pixmapOffset += event->pos() - lastDragPos;
        lastDragPos = event->pos(); //得到鼠标移动后的新位置
        update();
    }
}

//鼠标释放
void MandelbrotWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton){ //鼠标左键
        pixmapOffset += event->pos() - lastDragPos;
        lastDragPos = QPoint();
        //auto实现类型推导
        const auto pixmapSize = pixmap.size() / pixmap.devicePixelRatioF();
        int deltaX = (width() - pixmapSize.width()) / 2 - pixmapOffset.x();
        int deltaY = (height() - pixmapSize.height()) / 2 - pixmapOffset.y();
        scroll(deltaX, deltaY); //上下左右移动
    }
}

#if QT_CONFIG(wheelevent)
//滚轮事件
void MandelbrotWidget::wheelEvent(QWheelEvent *event){
    //返回轮子旋转的距离，以八分之一度为单位
    const int numDegrees = event->angleDelta().y() / 8;
    const double numSteps = numDegrees / double(15);
    zoom(pow(ZoomInFactor,numSteps)); //缩放
}
#endif

//更新像素图
void MandelbrotWidget::updatePixmap(const QImage &image, double scaleFactor)
{
    if(!lastDragPos.isNull())
        return;

    pixmap = QPixmap::fromImage(image); //将给定的图像转为像素图
    pixmapOffset = QPoint(); //偏移量
    lastDragPos = QPoint();
    pixmapScale = scaleFactor;
    update();
}

//缩放
void MandelbrotWidget::zoom(double zoomFactor)
{
    curScale *= zoomFactor;
    update();
    //重新设置绘制大小
    thread.render(centerX,centerY,curScale,size(),devicePixelRatioF());
}

//上下左右移动
void MandelbrotWidget::scroll(int deltaX, int deltaY)
{
    centerX += deltaX * curScale;
    centerY += deltaY * curScale;
    update();
    //重新设置大小
    thread.render(centerX,centerY,curScale,size(),devicePixelRatioF());
}
