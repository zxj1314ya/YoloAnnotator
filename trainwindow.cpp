#include "trainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>

TrainWindow::TrainWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("YOLO 模型训练");
    resize(700, 500);

    // 初始化 QProcess
    trainProcess = new QProcess(this);

    // 合并标准输出和错误输出（可选，这里我们分开处理以便标红错误信息）
    // trainProcess->setProcessChannelMode(QProcess::MergedChannels);

    setupUI();

    // 绑定信号槽：实时读取输出
    connect(trainProcess, &QProcess::readyReadStandardOutput, this, &TrainWindow::readProcessOutput);
    connect(trainProcess, &QProcess::readyReadStandardError, this, &TrainWindow::readProcessError);
    // 绑定信号槽：进程结束
    connect(trainProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TrainWindow::processFinished);
}

TrainWindow::~TrainWindow() {
    // 确保窗口关闭时，如果进程还在跑，就终止它
    if (trainProcess->state() == QProcess::Running) {
        trainProcess->kill();
        trainProcess->waitForFinished();
    }
}

void TrainWindow::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 1. 数据集与脚本选择区域
    QFormLayout* fileLayout = new QFormLayout();

    QHBoxLayout* dataLayout = new QHBoxLayout();
    dataPathEdit = new QLineEdit();
    QPushButton* dataBtn = new QPushButton("浏览...");
    dataLayout->addWidget(dataPathEdit);
    dataLayout->addWidget(dataBtn);
    fileLayout->addRow("数据集 (data.yaml):", dataLayout);

    QHBoxLayout* scriptLayout = new QHBoxLayout();
    scriptPathEdit = new QLineEdit("train.py"); // 默认假设脚本叫 train.py
    QPushButton* scriptBtn = new QPushButton("浏览...");
    scriptLayout->addWidget(scriptPathEdit);
    scriptLayout->addWidget(scriptBtn);
    fileLayout->addRow("Python 训练脚本:", scriptLayout);

    mainLayout->addLayout(fileLayout);

    // 2. 训练参数设置区域
    QFormLayout* paramLayout = new QFormLayout();

    epochsSpinBox = new QSpinBox();
    epochsSpinBox->setRange(1, 10000);
    epochsSpinBox->setValue(100);
    paramLayout->addRow("Epochs (训练轮数):", epochsSpinBox);

    batchSizeSpinBox = new QSpinBox();
    batchSizeSpinBox->setRange(1, 128);
    batchSizeSpinBox->setValue(16);
    paramLayout->addRow("Batch Size (批大小):", batchSizeSpinBox);

    modelComboBox = new QComboBox();
    modelComboBox->addItems({"yolov8n.pt", "yolov8s.pt", "yolov8m.pt"});
    paramLayout->addRow("预训练模型:", modelComboBox);

    mainLayout->addLayout(paramLayout);

    // 3. 控制按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    startBtn = new QPushButton("▶ 启动训练");
    startBtn->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold; padding: 8px;");
    stopBtn = new QPushButton("⏹ 停止训练");
    stopBtn->setStyleSheet("background-color: #f44336; color: white; font-weight: bold; padding: 8px;");
    stopBtn->setEnabled(false); // 默认不可用

    btnLayout->addWidget(startBtn);
    btnLayout->addWidget(stopBtn);
    mainLayout->addLayout(btnLayout);

    // 4. 实时日志显示区域
    QLabel* logLabel = new QLabel("实时控制台输出:");
    mainLayout->addWidget(logLabel);

    logConsole = new QTextEdit();
    logConsole->setReadOnly(true);
    logConsole->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: Consolas;");
    mainLayout->addWidget(logConsole);

    // 绑定按钮事件
    connect(dataBtn, &QPushButton::clicked, this, &TrainWindow::browseDataset);
    connect(scriptBtn, &QPushButton::clicked, this, &TrainWindow::browsePythonScript);
    connect(startBtn, &QPushButton::clicked, this, &TrainWindow::startTraining);
    connect(stopBtn, &QPushButton::clicked, this, &TrainWindow::stopTraining);
}

void TrainWindow::browseDataset() {
    QString path = QFileDialog::getOpenFileName(this, "选择 YOLO 数据集 YAML 文件", "", "YAML Files (*.yaml *.yml)");
    if (!path.isEmpty()) dataPathEdit->setText(path);
}

void TrainWindow::browsePythonScript() {
    QString path = QFileDialog::getOpenFileName(this, "选择 Python 训练脚本", "", "Python Scripts (*.py)");
    if (!path.isEmpty()) scriptPathEdit->setText(path);
}

void TrainWindow::startTraining() {
    if (dataPathEdit->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择数据集配置文件！");
        return;
    }

    logConsole->clear();
    logConsole->append("<font color='green'>>>> 准备启动训练...</font>");

    // 组装调用命令行：python -u train.py --data data.yaml --epochs 100 --batch 16
    QString pythonExe = "python"; // 确保你的环境变量里有 python，或者这里写死绝对路径
    QStringList arguments;

    // 【最关键的一步】：-u 强制无缓冲输出，否则你在 Qt 里看不到实时日志
    arguments << "-u";

    // 脚本路径
    arguments << scriptPathEdit->text();

    // 自定义参数传递给 python
    arguments << "--data" << dataPathEdit->text();
    arguments << "--epochs" << QString::number(epochsSpinBox->value());
    arguments << "--batch" << QString::number(batchSizeSpinBox->value());
    arguments << "--model" << modelComboBox->currentText();

    logConsole->append("<font color='gray'>执行命令: " + pythonExe + " " + arguments.join(" ") + "</font><br>");

    // 切换 UI 状态
    startBtn->setEnabled(false);
    stopBtn->setEnabled(true);

    // 启动进程
    trainProcess->start(pythonExe, arguments);

    if (!trainProcess->waitForStarted()) {
        logConsole->append("<font color='red'>错误：无法启动 Python 环境。请检查是否安装了 Python 且已配置环境变量。</font>");
        startBtn->setEnabled(true);
        stopBtn->setEnabled(false);
    }
}

void TrainWindow::stopTraining() {
    if (trainProcess->state() == QProcess::Running) {
        trainProcess->kill();
        logConsole->append("<font color='red'>>>> 训练已由用户强制终止。</font>");
    }
}

void TrainWindow::readProcessOutput() {
    // 读取标准输出并追加到文本框
    QByteArray out = trainProcess->readAllStandardOutput();
    //logConsole->append(QString::fromLocal8Bit(out)); // 处理中文乱码建议用 fromLocal8Bit 或 fromUtf8
    logConsole->append(QString::fromUtf8(out));

    // 滚动条自动滚动到最底部
    QScrollBar *scrollbar = logConsole->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

void TrainWindow::readProcessError() {
    // 读取错误输出（YOLO的进度条有时会输出到 error 通道）
    QByteArray err = trainProcess->readAllStandardError();
    // 错误信息标红显示
    //logConsole->append("<font color='#FF6B68'>" + QString::fromLocal8Bit(err).toHtmlEscaped() + "</font>");
    logConsole->append("<font color='#FF6B68'>" + QString::fromUtf8(err).toHtmlEscaped() + "</font>");

    QScrollBar *scrollbar = logConsole->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

void TrainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    startBtn->setEnabled(true);
    stopBtn->setEnabled(false);

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        logConsole->append("<br><font color='#4CAF50'><b>>>> 恭喜！模型训练圆满完成！</b></font>");
        QMessageBox::information(this, "完成", "模型训练已结束！");
    } else {
        logConsole->append("<br><font color='red'><b>>>> 训练异常退出，返回码: " + QString::number(exitCode) + "</b></font>");
    }
}