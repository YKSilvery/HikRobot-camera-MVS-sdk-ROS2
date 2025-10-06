# ROS2 Industrial Camera Driver

这是一个基于海康威视MVS SDK v4.6.0的ROS2工业相机驱动包。该包是自包含的，不需要系统级别的SDK安装。
所以你直接build就完事啦，没装MVS都行

我😭最近有事不在学校所以借不到相机😭所以使用的是虚拟相机调的，不知道strkey能不能正确应用到真相机上，不过参数肯定是写了的

## 功能特性

- 支持GigE和USB工业相机
- 动态参数控制（帧率、曝光时间、增益、像素格式）
- 自动图像格式检测和转换
- **智能断线重连**：设备断开时自动持续尝试重连，直到连接成功
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

#### 使用launch文件（推荐）

```bash
# 使用默认参数启动
ros2 launch camera camera.launch.py

# 或自定义参数启动
ros2 launch camera camera.launch.py frame_rate:=30.0 exposure_time:=5000.0 gain:=5.0 pixel_format:=RGB8Packed
```

#### 直接运行节点

```bash
ros2 run camera test_node
```

### 参数配置

#### 启动时参数配置（launch方式）

launch文件支持以下参数：

- `frame_rate`: 相机帧率，默认为1000.0 (fps)
- `exposure_time`: 曝光时间，默认为10000.0 (微秒)
- `gain`: 增益，默认为0.0 (dB)
- `pixel_format`: 像素格式，默认为"Mono8"

#### 运行时动态参数控制

支持以下参数的运行时调整：

- `frame_rate`: 帧率 (fps)
- `exposure_time`: 曝光时间 (微秒)
- `gain`: 增益 (dB)
- `pixel_format`: 像素格式 (支持的格式见下表)

#### 设置参数示例

```bash
# 设置帧率
ros2 param set /my_node frame_rate 30.0

# 设置曝光时间
ros2 param set /my_node exposure_time 5000.0

# 设置增益
ros2 param set /my_node gain 5.0

# 设置像素格式
ros2 param set /my_node pixel_format "RGB8Packed"
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

根据MVS SDK文档，支持以下像素格式（参数名）：

### 单通道格式
- `Mono8`: 8位单通道
- `Mono10`: 10位单通道
- `Mono10Packed`: 10位单通道（打包格式）
- `Mono12`: 12位单通道
- `Mono12Packed`: 12位单通道（打包格式）
- `Mono16`: 16位单通道

### 彩色格式
- `RGB8Packed`: 24位RGB彩色
- `BGR8Packed`: 24位BGR彩色
- `RGBA8Packed`: 32位RGBA彩色
- `BGRA8Packed`: 32位BGRA彩色
- `RGB10Packed`: 30位RGB彩色（打包）
- `RGB12Packed`: 36位RGB彩色（打包）

### Bayer格式
- `BayerRG8`: RG Bayer 8位
- `BayerGB8`: GB Bayer 8位
- `BayerGR8`: GR Bayer 8位
- `BayerBG8`: BG Bayer 8位
- `BayerGB10`: GB Bayer 10位
- `BayerGB12`: GB Bayer 12位
- `BayerGB12Packed`: GB Bayer 12位（打包）

### YUV格式
- `YUV422_8`: YUV422 8位
- `YUV422_8_UYVY`: YUV422 UYVY格式

**注意**: 不同相机型号支持的像素格式可能有差异，请以实际相机为准。

## 注意事项

- 增益控制在虚拟相机上可能不可用
- 确保相机连接并正确配置
- 包大小设置会自动优化（GigE相机）
- **断线重连机制**：设备断开时会每2秒自动尝试重连，直到连接成功为止

## 依赖项

- ROS2 Humble 或更高版本
- rclcpp
- sensor_msgs

## 自包含特性

该包包含了完整的MVS SDK库文件，无需系统级别安装。所有必要的库文件都会随包一起安装和分发。