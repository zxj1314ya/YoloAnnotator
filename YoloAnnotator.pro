QT       += core gui widgets

TARGET = YoloAnnotator
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    annotatescene.cpp \
    quantizewindow.cpp \
    trainwindow.cpp

HEADERS += \
    mainwindow.h \
    bndboxitem.h \
    annotatescene.h \
    quantizewindow.h \
    trainwindow.h


# ==========================================================
# 【新增】ONNX Runtime 1.25.1 配置 (注意 $$quote 和正斜杠的用法)
# ==========================================================
INCLUDEPATH += $$quote(D:/program application/onnxruntime-win-x64-1.25.1/include)

LIBS += -L$$quote(D:/program application/onnxruntime-win-x64-1.25.1/lib) -lonnxruntime


# ==========================================================
# 【恢复】OpenCV 4.5.5 配置 (图片预处理和 NMS 依然强烈依赖它！)
# ==========================================================
INCLUDEPATH += D:/OpenCV-MinGW-Build-OpenCV-4.5.5-x64/include

LIBS += -LD:/OpenCV-MinGW-Build-OpenCV-4.5.5-x64/x64/mingw/lib \
        -lopencv_core455 \
        -lopencv_imgproc455 \
        -lopencv_highgui455 \
        -lopencv_dnn455 \
        -lopencv_imgcodecs455