```mermaid
graph LR
    subgraph Application["Application Layer"]
        App[Application Singleton]
        StateMachine[Device State Machine]
        TaskScheduler[Task Scheduler]
    end

    subgraph Audio["Audio Processing"]
        AudioProc[Audio Processor]
        OpusCodec[Opus Codec]
        WakeWord[Wake Word Detection]
        VoiceDetect[Voice Detection]
    end

    subgraph Communication["Communication"]
        Protocol[Protocol Handler]
        OTA[OTA Manager]
        IoT[IoT State Manager]
    end

    subgraph Hardware["Hardware Abstraction"]
        LED[LED Controller]
        Display[Display Controller]
        Board[Board Support]
    end

    subgraph System["System Services"]
        FreeRTOS[FreeRTOS]
        EventGroup[Event Groups]
        Timer[ESP Timer]
    end

    %% Connections
    App --> StateMachine
    App --> TaskScheduler
    App --> AudioProc
    App --> Protocol
    App --> LED
    App --> Display

    AudioProc --> OpusCodec
    AudioProc --> WakeWord
    AudioProc --> VoiceDetect

    Protocol --> OTA
    Protocol --> IoT

    TaskScheduler --> FreeRTOS
    TaskScheduler --> EventGroup
    TaskScheduler --> Timer

    %% Device States
    subgraph States["Device States"]
        direction TB
        Starting[Starting]
        WifiConfig[WiFi Configuring]
        Idle[Idle]
        Connecting[Connecting]
        Listening[Listening]
        Speaking[Speaking]
        Upgrading[Upgrading]
        Activating[Activating]
        Error[Fatal Error]
    end

    StateMachine --> Starting
    StateMachine --> WifiConfig
    StateMachine --> Idle
    StateMachine --> Connecting
    StateMachine --> Listening
    StateMachine --> Speaking
    StateMachine --> Upgrading
    StateMachine --> Activating
    StateMachine --> Error

    %% Styling
    classDef default fill:#f9f9f9,stroke:#333,stroke-width:2px;
    classDef module fill:#e1f5fe,stroke:#0288d1,stroke-width:2px;
    classDef state fill:#fff3e0,stroke:#ff9800,stroke-width:2px;
    
    class Application,Audio,Communication,Hardware,System module;
    class States state;
```

# 系统架构说明

## 1. 应用层 (Application Layer)
- **Application Singleton**: 系统的核心控制器，管理所有模块的交互
- **Device State Machine**: 管理设备状态转换
- **Task Scheduler**: 负责任务调度和管理

## 2. 音频处理 (Audio Processing)
- **Audio Processor**: 音频处理核心
- **Opus Codec**: 音频编解码
- **Wake Word Detection**: 唤醒词检测
- **Voice Detection**: 语音检测

## 3. 通信模块 (Communication)
- **Protocol Handler**: 协议处理
- **OTA Manager**: 固件更新管理
- **IoT State Manager**: IoT状态管理

## 4. 硬件抽象层 (Hardware Abstraction)
- **LED Controller**: LED控制
- **Display Controller**: 显示控制
- **Board Support**: 板级支持

## 5. 系统服务 (System Services)
- **FreeRTOS**: 实时操作系统
- **Event Groups**: 事件组管理
- **ESP Timer**: 定时器服务

## 6. 设备状态 (Device States)
- **Starting**: 启动状态
- **WiFi Configuring**: WiFi配置状态
- **Idle**: 空闲状态
- **Connecting**: 连接状态
- **Listening**: 监听状态
- **Speaking**: 说话状态
- **Upgrading**: 升级状态
- **Activating**: 激活状态
- **Fatal Error**: 错误状态 