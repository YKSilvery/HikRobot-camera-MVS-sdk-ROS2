# ROS2 海康威视工业相机驱动

这是一个基于海康威视MVS SDK v4.6.0的ROS2工业相机驱动包。该包是自包含的，不需要系统级别的SDK安装，支持虚拟相机调试和真实工业相机部署。

## 🎯 核心特性

- **自包含设计**: 集成完整的MVS SDK库文件，无需系统级安装
- **多相机支持**: 支持GigE和USB工业相机
- **动态参数控制**: 运行时动态调整帧率、曝光时间、增益、像素格式
- **智能重连机制**: 设备断开时自动持续重连，无需手动干预
- **高效架构**: 基于ComposableNode的进程内通信，性能优化
- **实时监控**: 内置FPS统计和图像信息显示
- **自动可视化**: 一键启动RViz2进行图像预览
- **多格式支持**: 支持20+种像素格式的自动ROS消息转换

## 📦 快速开始

### 环境要求

- **ROS2版本**: Humble 或更高版本
- **系统**: Ubuntu 20.04/22.04 (推荐)
- **依赖**: rclcpp, sensor_msgs, rclcpp_components

### 构建安装

```bash
# 克隆或复制项目到工作空间
cd ~/ros2_ws/src

# 构建包 (自动包含所有SDK依赖)
colcon build --packages-select camera

# 激活环境
source install/setup.bash
```

### 一键启动 (推荐)

```bash
# 启动完整系统 (相机 + FPS监控 + RViz2可视化)
ros2 launch camera camera.launch.py
```

启动后会自动：
- 连接相机并开始采集
- 显示实时FPS统计
- 启动RViz2显示图像

## 🔧 参数配置

### 默认参数

系统启动时的默认参数配置：

| 参数 | 默认值 | 单位 | 描述 |
|------|--------|------|------|
| `frame_rate` | 1000.0 | fps | 相机帧率 |
| `exposure_time` | 10000.0 | μs | 曝光时间 |
| `gain` | 0.0 | dB | 增益 |
| `pixel_format` | "Mono8" | - | 像素格式 |

### 运行时动态调整

**注意：launch文件不支持参数设置，因为我拉了，只能通过ROS2参数命令动态修改**

支持运行时参数修改，无需重启节点：

```bash
# 调整帧率
ros2 param set /my_node frame_rate 500.0

# 调整曝光时间
ros2 param set /my_node exposure_time 15000.0

# 调整增益
ros2 param set /my_node gain 3.0

# 切换像素格式
ros2 param set /my_node pixel_format "RGB8Packed"
```

### 参数修改示例

```bash
# 高帧率模式设置
ros2 param set /my_node frame_rate 500.0

# 高质量模式设置
ros2 param set /my_node exposure_time 20000.0
ros2 param set /my_node gain 5.0

# 彩色模式设置
ros2 param set /my_node pixel_format "RGB8Packed"
```

## 📊 实时监控

系统提供完整的运行状态监控：

### FPS统计
- 每秒实时显示实际帧率
- 自动计算并显示统计信息

### 图像信息
- 每500帧显示图像尺寸和编码格式
- 帧序号跟踪

### 连接状态
- 设备连接状态监控
- 自动重连机制状态

## 🎨 支持的像素格式

### 单通道格式
- `Mono8`, `Mono10`, `Mono12`, `Mono16`
- `Mono10Packed`, `Mono12Packed`

### 彩色格式
- `RGB8Packed`, `BGR8Packed`
- `RGBA8Packed`, `BGRA8Packed`
- `RGB10Packed`, `RGB12Packed`

### Bayer格式
- `BayerRG8`, `BayerGB8`, `BayerGR8`, `BayerBG8`
- `BayerGB10`, `BayerGB12`, `BayerGB12Packed`

### YUV格式
- `YUV422_8`, `YUV422_8_UYVY`

## 🏗️ 系统架构

### ComposableNode设计

```
Camera Container
├── MyNode (相机驱动)
│   ├── 设备连接管理
│   ├── 参数动态配置
│   ├── 图像采集回调
│   └── 自动重连机制
└── ImageViewerNode (监控节点)
    ├── FPS实时统计
    └── 图像信息显示
```

### 通信机制

- **进程内通信**: 零拷贝，最高效
- **话题**: `/image_raw` (sensor_msgs/Image)
- **坐标系**: `camera` frame

## 🔍 可视化

### RViz2自动启动

launch文件自动启动RViz2，预配置：
- Image显示插件
- `/image_raw`话题订阅
- 优化的显示参数

### 手动查看

```bash
# 使用rqt_image_view
ros2 run rqt_image_view rqt_image_view

# 或单独启动RViz2
ros2 run rviz2 rviz2 -d install/camera/share/camera/rviz/camera.rviz
```

## ⚡ 性能优化

- **内存拷贝优化**: 使用`memcpy`直接复制图像数据
- **队列深度**: 自动优化缓冲区大小
- **包大小**: GigE相机自动设置最优包大小
- **回调驱动**: 基于SDK回调的实时采集

## 🔄 智能重连

设备断开时自动触发：
1. 检测到断开异常
2. 停止当前采集
3. 每2秒自动尝试重连
4. 重连成功后恢复所有参数设置
5. 重新开始图像采集

## 📝 使用说明

### 虚拟相机调试

当前使用虚拟相机进行开发调试：
- 模拟真实相机行为
- 支持所有参数配置
- 便于算法开发和测试

### 真实相机部署

切换到真实相机时：
1. 确保相机连接正常
2. 参数设置会自动应用到真实设备
3. 重连机制在真实环境中同样有效

## 🛠️ 故障排除

### 常见问题

**Q: RViz2显示空白**
A: 检查Fixed Frame设置，确保为"camera"或"map"

**Q: 参数修改无效**
A: 确认参数路径正确，虚拟相机可能不支持某些参数

**Q: 连接失败**
A: 检查相机连接，确认设备未被其他软件占用

**Q: 帧率不稳定**
A: 调整曝光时间，检查相机性能限制

### 日志分析

```bash
# 查看详细日志
ros2 launch camera camera.launch.py --ros-args --log-level debug

```bash
# 查看当前参数值
ros2 param list /my_node

# 获取特定参数值
ros2 param get /my_node frame_rate
```
```

## 📄 API参考

### 发布的话题

- `/image_raw` (sensor_msgs/Image): 原始图像数据

### 服务和动作

暂无

### 参数

详见"参数配置"章节
