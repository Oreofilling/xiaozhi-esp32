import librosa
import opuslib
import struct
import numpy as np

def create_hello_opus():
    # 创建一个简单的"你好"音频数据
    # 采样率 16kHz，持续时间 1 秒
    sample_rate = 16000
    duration = 1.0
    t = np.linspace(0, duration, int(sample_rate * duration))
    
    # 生成一个简单的音频波形（这里用正弦波模拟语音）
    audio = np.sin(2 * np.pi * 440 * t) * 0.5  # 440Hz 音调
    
    # 转换为 int16 格式
    audio = (audio * 32767).astype(np.int16)
    
    # 初始化 Opus 编码器
    encoder = opuslib.Encoder(sample_rate, 1, opuslib.APPLICATION_AUDIO)
    
    # 编码参数
    duration_ms = 60  # 60ms 每帧
    frame_size = int(sample_rate * duration_ms / 1000)
    
    # 生成第一帧数据
    frame = audio[:frame_size]
    opus_data = encoder.encode(frame.tobytes(), frame_size=frame_size)
    
    # 构造数据包
    packet = struct.pack('>BBH', 0, 0, len(opus_data)) + opus_data
    
    return packet

# 生成数据包
opus_packet = create_hello_opus()

# 打印十六进制格式
print("Opus packet (hex):")
print(' '.join(f'0x{b:02x},' for b in opus_packet))
