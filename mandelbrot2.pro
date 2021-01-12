QT += widgets

HEADERS += \
    mandelbrotwidget.h \
    renderthread.h

SOURCES += \
    main.cpp \
    mandelbrotwidget.cpp \
    renderthread.cpp

##这代码影响样式。LIBS指定要链接到项目中的库列表。-lm表示连接系统的数学库
unix:!mac:!vxworks:!integrity:!haiku:LIBS += -lm
