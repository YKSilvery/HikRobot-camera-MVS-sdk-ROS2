# ROS2 Industrial Camera Driver

这是一个基于海康威视MVS SDK v4.6.0的ROS2工业相机驱动包。该包是自包含的，不需要系统级别的SDK安装。
所以你直接build就完事啦，没装MVS都行

## 功能特性

- 支持GigE和USB工业相机
- 动态参数控制（帧率、曝光时间、增益、像素格式）
- 自动图像格式检测和转换
- 设备断开自动重连
- 基于回调的图像采集（高效）

## 构建和安装

```bash
# 克隆或复制项目到工作空间
cd ~/ros2_ws/src

# 构建包
colcon build --packages-select camera

# 安装环境
source install/setup.bash
```

## 使用方法

### 启动相机节点

```bash
ros2 run camera test_node
```

### 动态参数控制

支持以下参数的运行时调整：

- `frame_rate`: 帧率 (fps)
- `exposure_time`: 曝光时间 (微秒)
- `gain`: 增益 (dB)
- `pixel_format`: 像素格式 (Mono8, RGB8, etc.)

#### 设置参数示例

```bash
# 设置帧率
ros2 param set /my_node frame_rate 30.0

# 设置曝光时间
ros2 param set /my_node exposure_time 5000.0

# 设置增益
ros2 param set /my_node gain 5.0

# 设置像素格式
ros2 param set /my_node pixel_format "RGB8"
```

### 图像话题

节点发布图像到 `/image_raw` 话题，消息类型为 `sensor_msgs/Image`。

#### 查看图像

```bash
# 使用rqt_image_view
ros2 run rqt_image_view rqt_image_view

# 或使用rviz2
ros2 run rviz2 rviz2
# 在rviz2中添加Image显示插件，选择/image_raw话题
```

## 支持的像素格式

- Mono8, Mono10, Mono12
- RGB8, BGR8, RGBA8, BGRA8
- Bayer格式 (RGGB, GRBG, GBRG, BGGR)
- RGB10, RGB12

## 注意事项

- 增益控制在虚拟相机上可能不可用
- 确保相机连接并正确配置
- 包大小设置会自动优化（GigE相机）

## 依赖项

- ROS2 Humble 或更高版本
- rclcpp
- sensor_msgs

## 自包含特性

该包包含了完整的MVS SDK库文件，无需系统级别安装。所有必要的库文件都会随包一起安装和分发。