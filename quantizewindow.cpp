#include "quantizewindow.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QProcessEnvironment>
#include <QScrollBar>
#include <QMessageBox>
#include <QRegularExpression>

QuantizeWindow::QuantizeWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("模型量化与导出");
    resize(600, 400);

    qProcess = new QProcess(this);

    // 【修改 1】合并标准输出和标准错误，这样即使底层报错，黑框里也能显示出来！
    qProcess->setProcessChannelMode(QProcess::MergedChannels);

    // 强制输出为 UTF-8，防止乱码
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PYTHONIOENCODING", "utf-8");
    qProcess->setProcessEnvironment(env);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QFormLayout* form = new QFormLayout();

    inputModelEdit = new QLineEdit("D:/tmp/YoloAnnotator/build/Desktop_Qt_6_8_3_MinGW_64_bit-Debug/debug/runs/detect/train3/weights/best.pt");
    halfCheck = new QCheckBox("FP16 半精度量化 (推荐, 速度快不掉精度)");
    halfCheck->setChecked(false);
    int8Check = new QCheckBox("INT8 极致量化 (暂未接入校准数据集，可能报错)");

    form->addRow("输入模型 (.pt):", inputModelEdit);
    form->addRow("精度压缩:", halfCheck);
    form->addRow("", int8Check);
    layout->addLayout(form);

    startBtn = new QPushButton("🚀 开始量化并加载");
    layout->addWidget(startBtn);

    logConsole = new QTextEdit();
    logConsole->setReadOnly(true);
    logConsole->setStyleSheet("background-color: black; color: #00FF00; font-family: Consolas;");
    layout->addWidget(logConsole);

    connect(startBtn, &QPushButton::clicked, this, &QuantizeWindow::startQuantization);
    connect(qProcess, &QProcess::readyReadStandardOutput, this, &QuantizeWindow::readOutput);
    connect(qProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &QuantizeWindow::processFinished);
}

QuantizeWindow::~QuantizeWindow() {
    if (qProcess->state() == QProcess::Running) {
        qProcess->kill();
        qProcess->waitForFinished();
    }
}

void QuantizeWindow::startQuantization() {
    if (inputModelEdit->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入模型路径");
        return;
    }

    logConsole->clear();
    startBtn->setEnabled(false);
    finalModelPath = "";

    QString ptPath = inputModelEdit->text();

    // 【修改 2】直接拼接 yolo 官方导出命令
    // QString cmdLine = QString("yolo export model=\"%1\" format=onnx").arg(ptPath);
    // if (halfCheck->isChecked()) cmdLine += " half=True";
    // if (int8Check->isChecked()) cmdLine += " int8=True";

    // 增加 opset=12 和 simplify=True 以兼容低版本 OpenCV
    QString cmdLine = QString("yolo export model=\"%1\" format=onnx opset=12 simplify=True").arg(ptPath);
    if (halfCheck->isChecked()) cmdLine += " half=True";
    if (int8Check->isChecked()) cmdLine += " int8=True";

    logConsole->append("<font color='gray'>执行量化命令: " + cmdLine + "</font><br>");

    // 【修改 3】使用 cmd /c 唤起，完美继承你在命令行里验证过的全局环境！
    qProcess->start("cmd", QStringList() << "/c" << cmdLine);
}

void QuantizeWindow::readOutput() {
    QByteArray out = qProcess->readAllStandardOutput();
    // Windows 的 cmd 输出有时是本地编码，防止出现乱码
    QString text = QString::fromLocal8Bit(out);

    logConsole->moveCursor(QTextCursor::End);
    logConsole->insertPlainText(text);
    logConsole->verticalScrollBar()->setValue(logConsole->verticalScrollBar()->maximum());
}

void QuantizeWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    startBtn->setEnabled(true);

    // 【修改 4】如果正常结束 (exitCode == 0)，我们自己推算 .onnx 的路径
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        finalModelPath = inputModelEdit->text();
        finalModelPath.replace(".pt", ".onnx"); // 把结尾的 .pt 替换为 .onnx

        logConsole->moveCursor(QTextCursor::End);
        logConsole->insertHtml("<br><font color='yellow'>>>> 量化完成！正在将 ONNX 模型交接至 C++ 原生推理引擎...</font>");

        // 核心：发出信号通知主窗口去加载模型
        emit modelQuantized(finalModelPath);
    } else {
        logConsole->moveCursor(QTextCursor::End);
        logConsole->insertHtml("<br><font color='red'>>>> 量化失败或被强行终止。请检查上方日志报错信息。</font>");
    }
}