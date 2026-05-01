#ifndef TRAINWINDOW_H
#define TRAINWINDOW_H

#include <QWidget>
#include <QProcess>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>

class TrainWindow : public QWidget {
    Q_OBJECT

public:
    explicit TrainWindow(QWidget *parent = nullptr);
    ~TrainWindow();

private slots:
    void browseDataset();       // 浏览数据集配置文件
    void browsePythonScript();  // 浏览训练脚本
    void startTraining();       // 启动训练
    void stopTraining();        // 停止训练
    void readProcessOutput();   // 实时读取标准输出
    void readProcessError();    // 实时读取错误输出
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus); // 训练结束处理

private:
    void setupUI();

    QProcess* trainProcess;     // 核心：外部进程对象

    // UI 控件
    QLineEdit* dataPathEdit;
    QLineEdit* scriptPathEdit;
    QSpinBox* epochsSpinBox;
    QSpinBox* batchSizeSpinBox;
    QComboBox* modelComboBox;
    QPushButton* startBtn;
    QPushButton* stopBtn;
    QTextEdit* logConsole;
};

#endif // TRAINWINDOW_H