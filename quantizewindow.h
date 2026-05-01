#ifndef QUANTIZEWINDOW_H
#define QUANTIZEWINDOW_H

#include <QWidget>
#include <QProcess>
#include <QLineEdit>
#include <QCheckBox>
#include <QTextEdit>
#include <QPushButton>

class QuantizeWindow : public QWidget {
    Q_OBJECT
public:
    explicit QuantizeWindow(QWidget *parent = nullptr);
    ~QuantizeWindow();

signals:
    // 量化成功后发出此信号，传递导出的 ONNX 模型路径给主窗口
    void modelQuantized(QString onnxPath);

private slots:
    void startQuantization();
    void readOutput();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess* qProcess;
    QLineEdit* inputModelEdit; // 输入的 .pt 模型名称/路径
    QCheckBox* halfCheck;      // FP16 选项
    QCheckBox* int8Check;      // INT8 选项
    QPushButton* startBtn;
    QTextEdit* logConsole;
    QString finalModelPath;    // 拦截 Python 脚本返回的终态路径
};

#endif // QUANTIZEWINDOW_H