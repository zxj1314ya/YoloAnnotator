#include "mainwindow.h"
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QTextStream>
#include <QDir>
#include <QFile>
#include <QActionGroup>
#include <QDockWidget>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    setStyleSheet("QMainWindow { background-color: #f0f2f5; }"
                  "QDockWidget { font-weight: bold; }"
                  "QListWidget { border: 1px solid #d9d9d9; background: white; font-size: 13px; }"
                  "QListWidget::item { padding: 5px; border-bottom: 1px solid #f0f0f0; }"
                  "QListWidget::item:selected { background-color: #e6f7ff; color: #1890ff; border-left: 3px solid #1890ff; }");
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    scene = new AnnotateScene(this);
    view = new AnnotateView(scene, this);
    setCentralWidget(view);

    QToolBar* mainToolBar = addToolBar("Main");
    mainToolBar->setMovable(false);

    panAction = mainToolBar->addAction("✋ 浏览平移 (W)");
    panAction->setCheckable(true);
    drawAction = mainToolBar->addAction("✏️ 画框标注 (E)");
    drawAction->setCheckable(true);

    panAction->setShortcut(QKeySequence(Qt::Key_W));
    drawAction->setShortcut(QKeySequence(Qt::Key_E));

    QActionGroup* modeGroup = new QActionGroup(this);
    modeGroup->addAction(panAction);
    modeGroup->addAction(drawAction);
    connect(modeGroup, &QActionGroup::triggered, this, &MainWindow::switchMode);

    mainToolBar->addSeparator();
    QAction* saveAction = mainToolBar->addAction("💾 保存当前标注 (Ctrl+S)");
    saveAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveYolo);

    mainToolBar->addSeparator();
    inferAction = mainToolBar->addAction("🧠 AI 自动识别 (需先加载模型)");
    inferAction->setEnabled(false);
    connect(inferAction, &QAction::triggered, this, &MainWindow::runInference);

    QMenu* toolMenu = menuBar()->addMenu("工具");
    QAction* trainAction = toolMenu->addAction("🚀 训练 YOLO 模型...");
    connect(trainAction, &QAction::triggered, this, [this]() {
        TrainWindow* trainWin = new TrainWindow(this);
        trainWin->setWindowFlag(Qt::Window);
        trainWin->show();
    });

    QAction* quantizeAction = toolMenu->addAction("⚙️ 模型量化与部署...");
    connect(quantizeAction, &QAction::triggered, this, &MainWindow::openQuantizeWindow);

    QDockWidget* dock = new QDockWidget("📁 文件列表", this);
    fileListWidget = new QListWidget(dock);
    dock->setWidget(fileListWidget);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    connect(fileListWidget, &QListWidget::itemClicked, this, &MainWindow::onFileSelected);

    QMenu* fileMenu = menuBar()->addMenu("文件");
    QAction* openImgAction = fileMenu->addAction("打开单张图片...");
    QAction* openDirAction = fileMenu->addAction("打开文件夹...");
    connect(openImgAction, &QAction::triggered, this, &MainWindow::openImage);
    connect(openDirAction, &QAction::triggered, this, &MainWindow::openDirectory);

    panAction->setChecked(true);
    switchMode(panAction);
}

void MainWindow::switchMode(QAction* action) {
    if (action == panAction) {
        view->setDragMode(QGraphicsView::ScrollHandDrag);
        scene->setDrawMode(false);
    } else if (action == drawAction) {
        view->setDragMode(QGraphicsView::NoDrag);
        scene->setDrawMode(true);
    }
}

void MainWindow::openImage() {
    QString fileName = QFileDialog::getOpenFileName(this, "打开图片", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (fileName.isEmpty()) return;
    QFileInfo fileInfo(fileName);
    currentDirPath = fileInfo.absolutePath();
    fileListWidget->clear();
    fileListWidget->addItem(fileInfo.fileName());
    fileListWidget->setCurrentRow(0);
    onFileSelected(fileListWidget->item(0));
}

void MainWindow::openDirectory() {
    QString dirPath = QFileDialog::getExistingDirectory(this, "选择文件夹");
    if (dirPath.isEmpty()) return;
    currentDirPath = dirPath;
    fileListWidget->clear();
    QDir dir(dirPath);
    QStringList filters = {"*.png", "*.jpg", "*.jpeg", "*.bmp"};
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);
    for (const QFileInfo& fileInfo : fileList) {
        fileListWidget->addItem(fileInfo.fileName());
    }
    if (fileListWidget->count() > 0) {
        fileListWidget->setCurrentRow(0);
        onFileSelected(fileListWidget->item(0));
    }
}

void MainWindow::onFileSelected(QListWidgetItem* item) {
    if (!item) return;
    QString fileName = item->text();
    currentImagePath = currentDirPath + "/" + fileName;

    QPixmap pixmap;
    QFile file(currentImagePath);
    if (file.open(QIODevice::ReadOnly)) {
        pixmap.loadFromData(file.readAll());
    }
    scene->setImage(pixmap);
    view->resetTransform();
    view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);

    QFileInfo fileInfo(currentImagePath);
    QString txtPath = fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".txt";
    if (QFile::exists(txtPath)) loadTxtAnnotation(txtPath, pixmap.width(), pixmap.height());
}

void MainWindow::loadTxtAnnotation(const QString& txtPath, int imgW, int imgH) {
    QFile file(txtPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if(line.isEmpty()) continue;
        QStringList parts = line.split(" ", Qt::SkipEmptyParts);
        if (parts.size() == 5) {
            BndBoxItem* box = new BndBoxItem(QRectF(parts[1].toDouble()*imgW - (parts[3].toDouble()*imgW/2),
                                                    parts[2].toDouble()*imgH - (parts[4].toDouble()*imgH/2),
                                                    parts[3].toDouble()*imgW, parts[4].toDouble()*imgH));
            box->setClassId(parts[0]);
            scene->addItem(box);
        }
    }
}

void MainWindow::saveYolo() {
    if (currentImagePath.isEmpty()) return;
    QFileInfo fileInfo(currentImagePath);
    QString txtPath = fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".txt";
    QFile file(txtPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    for (QGraphicsItem* item : scene->items()) {
        if (BndBoxItem* box = dynamic_cast<BndBoxItem*>(item)) {
            QRectF r = box->sceneBoundingRect();
            out << "0 " << (r.center().x()/scene->width()) << " " << (r.center().y()/scene->height())
                << " " << (r.width()/scene->width()) << " " << (r.height()/scene->height()) << "\n";
        }
    }
}

void MainWindow::openQuantizeWindow() {
    QuantizeWindow* qw = new QuantizeWindow(this);
    qw->setWindowFlag(Qt::Window);
    connect(qw, &QuantizeWindow::modelQuantized, this, &MainWindow::loadQuantizedModel);
    qw->show();
}

// void MainWindow::loadQuantizedModel(QString onnxPath) {
//     try {
//         yoloNet = cv::dnn::readNetFromONNX(onnxPath.toStdString());
//         yoloNet.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
//         yoloNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
//         isModelLoaded = true;
//         inferAction->setEnabled(true);
//         inferAction->setText("🧠 AI 自动识别 (已就绪)");
//         QMessageBox::information(this, "成功", "模型挂载成功！");
//     } catch (cv::Exception& e) {
//         QMessageBox::critical(this, "错误", "模型初始化失败：" + QString::fromStdString(e.what()));
//     }
// }

void MainWindow::loadQuantizedModel(QString onnxPath) {
    try {
        // 如果已经加载过模型，先清理旧内存
        if (ortSession) {
            delete ortSession;
            delete memoryInfo;
            delete ortEnv;
        }

        // 1. 初始化 ORT 环境
        ortEnv = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "YoloInference");
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(1); // 设置单线程或多线程
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

        // 2. 加载模型 (Windows 下必须转为 wstring)
        std::wstring wStrPath = onnxPath.toStdWString();
        ortSession = new Ort::Session(*ortEnv, wStrPath.c_str(), sessionOptions);

        // 3. 初始化内存分配器
        memoryInfo = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));

        isModelLoaded = true;
        inferAction->setEnabled(true);
        inferAction->setText("🧠 AI 自动识别 (ORT引擎已就绪)");
        QMessageBox::information(this, "成功", "ONNX Runtime 模型挂载成功！");
    } catch (const Ort::Exception& e) {
        QMessageBox::critical(this, "ORT 初始化错误", QString("模型加载失败：%1").arg(e.what()));
    }
}

// ===================== 终极推理函数 (包含强制弹窗调试) =====================
// void MainWindow::runInference() {
//     if (!isModelLoaded || currentImagePath.isEmpty()) return;

//     // 强制弹窗1：确认函数被触发
//     // QMessageBox::information(this, "Debug", "Inference Triggered!");

//     // 1. 读取图
//     QFile file(currentImagePath);
//     if (!file.open(QIODevice::ReadOnly)) return;
//     QByteArray data = file.readAll();
//     std::vector<uchar> buf(data.begin(), data.end());
//     cv::Mat frame = cv::imdecode(buf, cv::IMREAD_COLOR);
//     if (frame.empty()) {
//         QMessageBox::warning(this, "Debug", "Image Read Failed!");
//         return;
//     }

//     // 2. 预处理
//     cv::Mat blob;
//     cv::dnn::blobFromImage(frame, blob, 1.0 / 255.0, cv::Size(640, 640), cv::Scalar(), true, false);
//     yoloNet.setInput(blob);

//     // 3. 推理
//     std::vector<cv::Mat> outputs;
//     try {
//         yoloNet.forward(outputs, yoloNet.getUnconnectedOutLayersNames());
//     } catch (const cv::Exception& e) {
//         QMessageBox::critical(this, "Forward Error", QString::fromStdString(e.what()));
//         return;
//     }

//     if (outputs.empty()) {
//         QMessageBox::warning(this, "Debug", "Output is empty!");
//         return;
//     }

//     // 强制弹窗2：显示维度信息
//     cv::Mat output = outputs[0];
//     QString info = QString("Output Dims: %1 | Row1: %2 | Col1: %3")
//                        .arg(output.dims)
//                        .arg(output.size[1])
//                        .arg(output.size[2]);
//     QMessageBox::information(this, "Model Structure", info);

//     // 4. 解析
//     if (output.dims == 3) {
//         int sz[] = { output.size[1], output.size[2] };
//         output = cv::Mat(2, sz, CV_32F, output.data);
//     }
//     cv::Mat output_t = output.t();

//     std::vector<float> confidences;
//     std::vector<cv::Rect> boxes;
//     float x_factor = (float)frame.cols / 640.0f;
//     float y_factor = (float)frame.rows / 640.0f;

//     float* pdata = (float*)output_t.data;
//     for (int i = 0; i < output_t.rows; ++i) {
//         float max_score = 0;
//         // 自动查找类别分 (YOLOv8 之后的数据列)
//         for (int j = 4; j < output_t.cols; ++j) {
//             if (pdata[j] > max_score) max_score = pdata[j];
//         }

//         if (max_score > 0.1) { // 阈值极大程度降低用于测试
//             float cx = pdata[0], cy = pdata[1], w = pdata[2], h = pdata[3];
//             int left = int((cx - 0.5 * w) * x_factor);
//             int top = int((cy - 0.5 * h) * y_factor);
//             int width = int(w * x_factor);
//             int height = int(h * y_factor);
//             boxes.push_back(cv::Rect(left, top, width, height));
//             confidences.push_back(max_score);
//         }
//         pdata += output_t.cols;
//     }

//     // 5. NMS
//     std::vector<int> indices;
//     cv::dnn::NMSBoxes(boxes, confidences, 0.1f, 0.45f, indices);

//     if (indices.empty()) {
//         QMessageBox::information(this, "Result", "AI found 0 objects (Scores might be too low).");
//     } else {
//         QMessageBox::information(this, "Result", QString("AI found %1 objects!").arg(indices.size()));
//     }

//     for (int i : indices) {
//         cv::Rect b = boxes[i];
//         BndBoxItem* boxItem = new BndBoxItem(QRectF(b.x, b.y, b.width, b.height));
//         boxItem->setClassId(QString("Cell %.2f").arg(confidences[i]));
//         boxItem->setPen(QPen(Qt::blue, 2));
//         scene->addItem(boxItem);
//     }
// }

void MainWindow::runInference() {
    if (!isModelLoaded || !ortSession || currentImagePath.isEmpty()) return;

    // 1. 读取图像
    cv::Mat frame = cv::imread(currentImagePath.toLocal8Bit().toStdString());
    if (frame.empty()) {
        QMessageBox::warning(this, "错误", "无法读取当前图像！");
        return;
    }

    cv::Mat blob;
    cv::dnn::blobFromImage(frame, blob, 1.0 / 255.0, cv::Size(640, 640), cv::Scalar(), true, false);

    std::vector<int64_t> inputShape = {1, 3, 640, 640};
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
        *memoryInfo,
        (float*)blob.data, blob.total(),
        inputShape.data(), inputShape.size()
        );

    const char* inputNames[] = {"images"};
    const char* outputNames[] = {"output0"};

    // 2. 执行推理
    std::vector<Ort::Value> outputTensors;
    try {
        Ort::RunOptions runOptions; // 安全规范的运行参数初始化
        outputTensors = ortSession->Run(runOptions, inputNames, &inputTensor, 1, outputNames, 1);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "推理引擎报错", QString::fromStdString(e.what()));
        return;
    }

    if (outputTensors.empty()) return;

    // 3. ✨ 动态获取输出张量维度 (彻底解决内存越界崩溃！)
    Ort::TensorTypeAndShapeInfo shapeInfo = outputTensors[0].GetTensorTypeAndShapeInfo();
    std::vector<int64_t> outShape = shapeInfo.GetShape();

    if (outShape.size() < 3) {
        QMessageBox::warning(this, "模型异常", "输出张量维度不符合 YOLOv8 规范！");
        return;
    }

    // outShape 通常长这样：[1, elements, 8400]
    int numElements = outShape[1];   // 例如：5 (4坐标+1类别) 或 84 (4坐标+80类别)
    int numProposals = outShape[2];  // 8400 预测框

    float* outData = outputTensors[0].GetTensorMutableData<float>();

    // 4. 解析结果
    cv::Mat outputMat(numElements, numProposals, CV_32F, outData);
    outputMat = outputMat.t(); // 转置为 [8400, elements]，方便遍历

    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;
    std::vector<int> classIds; // 记录对应的类别 ID

    float x_factor = (float)frame.cols / 640.0f;
    float y_factor = (float)frame.rows / 640.0f;

    for (int i = 0; i < numProposals; ++i) {
        float* rowPtr = outputMat.ptr<float>(i);

        float maxScore = 0;
        int classId = -1;

        // 从索引 4 开始，循环读取所有的类别得分，找到最高分
        for (int j = 4; j < numElements; ++j) {
            if (rowPtr[j] > maxScore) {
                maxScore = rowPtr[j];
                classId = j - 4; // 还原类别索引 (0, 1, 2...)
            }
        }

        if (maxScore > 0.25) { // 过滤掉低于 25% 置信度的杂讯
            float cx = rowPtr[0];
            float cy = rowPtr[1];
            float w = rowPtr[2];
            float h = rowPtr[3];

            int left = int((cx - 0.5 * w) * x_factor);
            int top = int((cy - 0.5 * h) * y_factor);
            int width = int(w * x_factor);
            int height = int(h * y_factor);

            boxes.push_back(cv::Rect(left, top, width, height));
            confidences.push_back(maxScore);
            classIds.push_back(classId);
        }
    }

    // 5. 非极大值抑制 (NMS)，消除同物体重叠框
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, 0.25f, 0.45f, indices);

    if (indices.empty()) {
        QMessageBox::information(this, "结果", "AI 识别完毕，但未发现置信度达标的目标！");
        return;
    }

    // 6. 渲染到 Qt 场景
    for (int i : indices) {
        cv::Rect b = boxes[i];
        BndBoxItem* boxItem = new BndBoxItem(QRectF(b.x, b.y, b.width, b.height));

        // 显示识别到的类别 ID
        boxItem->setClassId(QString::number(classIds[i]));
        boxItem->setPen(QPen(Qt::blue, 2));
        scene->addItem(boxItem);
    }
}