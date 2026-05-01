#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QWheelEvent>
#include <QListWidget>
#include <QDockWidget>
#include <QActionGroup>

// ======= 【新增】引入 OpenCV 头文件 =======
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <onnxruntime_cxx_api.h>
// ==========================================

#include "annotatescene.h"
#include "trainwindow.h"
#include "quantizewindow.h" // 【新增】量化窗口头文件

class AnnotateView : public QGraphicsView {
public:
    AnnotateView(QGraphicsScene *scene, QWidget *parent = nullptr) : QGraphicsView(scene, parent) {
        setRenderHint(QPainter::Antialiasing);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    }
protected:
    void wheelEvent(QWheelEvent *event) override {
        if (event->angleDelta().y() > 0) scale(1.15, 1.15);
        else scale(1.0 / 1.15, 1.0 / 1.15);
    }
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openImage();             // 打开单张图片
    void openDirectory();         // 打开整个文件夹
    void onFileSelected(QListWidgetItem* item);
    void saveYolo();
    void switchMode(QAction* action);

    // ======= 【新增】模型量化与推理槽函数 =======
    void loadQuantizedModel(QString onnxPath); // 接收量化成功的信号并加载模型
    void runInference();                       // 执行 AI 推理
    void openQuantizeWindow();                 // 打开量化配置窗口
    // ==========================================

private:
    AnnotateView* view;
    AnnotateScene* scene;
    QListWidget* fileListWidget;
    QString currentDirPath;
    QString currentImagePath;

    QAction* panAction;
    QAction* drawAction;

    // ======= 【新增】推理相关变量 =======
    // cv::dnn::Net yoloNet;      // OpenCV DNN 网络对象

    Ort::Env* ortEnv = nullptr;
    Ort::Session* ortSession = nullptr;
    Ort::MemoryInfo* memoryInfo = nullptr;
    bool isModelLoaded = false;// 标记模型是否已加载
    QAction* inferAction;      // 菜单栏推理按钮
    // ===================================

    void setupUI();
    void loadTxtAnnotation(const QString& txtPath, int imgW, int imgH);
};

#endif // MAINWINDOW_H