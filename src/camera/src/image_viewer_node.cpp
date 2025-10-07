#include "rclcpp/rclcpp.hpp"
#include "rclcpp_components/register_node_macro.hpp"
#include "sensor_msgs/msg/image.hpp"

class ImageViewerNode : public rclcpp::Node
{
public:
  ImageViewerNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Node("image_viewer_node", options),
    frame_count_(0),
    last_fps_time_(this->now())
  {
    // 创建订阅者
    image_subscriber_ = this->create_subscription<sensor_msgs::msg::Image>(
      "image_raw", 10,
      std::bind(&ImageViewerNode::image_callback, this, std::placeholders::_1));
    
    // 创建定时器，每秒输出一次FPS
    fps_timer_ = this->create_wall_timer(
      std::chrono::seconds(1),
      std::bind(&ImageViewerNode::fps_callback, this));
    
    RCLCPP_INFO(this->get_logger(), "ImageViewerNode initialized, subscribing to image_raw");
  }

private:
  void image_callback(const sensor_msgs::msg::Image::SharedPtr msg)
  {
    frame_count_++;
    // 只在每100帧输出一次图像信息，避免刷屏
    if (frame_count_ % 500 == 0) {
      RCLCPP_INFO(this->get_logger(), "Received image: %dx%d, encoding: %s, frame_num: %u",
                  msg->width, msg->height, msg->encoding.c_str(), msg->header.stamp.nanosec);
    }
    // 这里可以添加图像处理或显示逻辑
  }

  void fps_callback()
  {
    auto current_time = this->now();
    auto duration = current_time - last_fps_time_;
    double duration_sec = duration.seconds();
    
    if (duration_sec > 0.0) {
      double fps = static_cast<double>(frame_count_) / duration_sec;
      RCLCPP_INFO(this->get_logger(), "=== VIEWER RECEIVED FPS: %.2f === (frames: %d in %.2f seconds)", 
                  fps, frame_count_, duration_sec);
    }
    
    // 重置计数器
    frame_count_ = 0;
    last_fps_time_ = current_time;
  }

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_subscriber_;
  rclcpp::TimerBase::SharedPtr fps_timer_;
  int frame_count_;
  rclcpp::Time last_fps_time_;
};

RCLCPP_COMPONENTS_REGISTER_NODE(ImageViewerNode)