import argparse
import sys
import time
from ultralytics import YOLO

# 强制刷新输出流，配合 Qt 的 QProcess 实时读取
def print_flush(text):
    print(text)
    sys.stdout.flush()

if __name__ == '__main__':
    # 1. 接收来自 Qt GUI 传过来的参数
    parser = argparse.ArgumentParser()
    parser.add_argument('--data', type=str, required=True, help='数据集 yaml 路径')
    parser.add_argument('--epochs', type=int, default=100)
    parser.add_argument('--batch', type=int, default=16)
    parser.add_argument('--model', type=str, default='yolov8n.pt')
    args = parser.parse_args()

    print_flush(f"========== 收到 Qt 前端训练请求 ==========")
    print_flush(f"模型版本: {args.model}")
    print_flush(f"数据集配置: {args.data}")
    print_flush(f"训练轮数 (Epochs): {args.epochs}")
    print_flush(f"批大小 (Batch): {args.batch}")
    print_flush("=========================================")
    
    time.sleep(1) # 停顿一下，让前端稍微看清初始信息

    try:
        # 2. 初始化 YOLO 模型
        print_flush(">>> 正在加载模型权重...")
        model = YOLO(args.model)
        
        # 3. 启动训练
        print_flush(">>> 模型加载完毕，开始训练！")
        # 注意：YOLO 自身的训练日志也会输出，Qt 会实时捕获
        results = model.train(
            data=args.data, 
            epochs=args.epochs, 
            batch=args.batch,
            verbose=True
        )
        print_flush(">>> 训练任务全部圆满结束！")
        
    except Exception as e:
        # 如果出错，把错误信息也打印出来给 Qt 看
        print(f"训练发生错误: {e}", file=sys.stderr)
        sys.exit(1)