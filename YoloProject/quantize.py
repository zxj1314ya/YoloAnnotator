import argparse
import sys
import time
from ultralytics import YOLO

# 强制无缓冲输出，给 Qt 实时抓取
def print_flush(text):
    print(text)
    sys.stdout.flush()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--weights', type=str, required=True, help='输入的 .pt 模型路径')
    parser.add_argument('--format', type=str, default='onnx', help='导出格式')
    parser.add_argument('--half', action='store_true', help='是否开启 FP16 半精度量化')
    parser.add_argument('--int8', action='store_true', help='是否开启 INT8 量化')
    parser.add_argument('--data', type=str, default='', help='INT8 量化需要的校准数据集 yaml')
    args = parser.parse_args()

    print_flush("========== 收到 Qt 模型量化请求 ==========")
    print_flush(f"输入模型: {args.weights}")
    print_flush(f"目标格式: {args.format}")
    print_flush(f"FP16 半精度: {args.half}")
    print_flush(f"INT8 量化: {args.int8}")
    print_flush("=========================================")
    
    try:
        print_flush(">>> 正在加载原始模型...")
        model = YOLO(args.weights)
        
        print_flush(">>> 开始执行量化与导出工作 (这可能需要几分钟)...")
        # YOLOv8 导出 API
        export_args = {'format': args.format}
        if args.half: export_args['half'] = True
        if args.int8: 
            export_args['int8'] = True
            if args.data: export_args['data'] = args.data
            
        export_path = model.export(**export_args)
        
        print_flush(f">>> 量化导出圆满完成！")
        print_flush(f"!!!SUCCESS_PATH:{export_path}") # 给 Qt 传递最终路径的特定标记
        
    except Exception as e:
        print(f"量化过程发生错误: {e}", file=sys.stderr)
        sys.exit(1)