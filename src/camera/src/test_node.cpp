#include "rclcpp/rclcpp.hpp"
#include "rclcpp_components/register_node_macro.hpp"
#include "MvCameraControl.h"
#include <cstring>
#include <thread>
#include <chrono>
#include "sensor_msgs/msg/image.hpp"

class MyNode : public rclcpp::Node
{
public:

  // 像素格式到ROS encoding的映射函数
  static std::string pixelTypeToEncoding(MvGvspPixelType pixelType) {
    switch (pixelType) {
      case PixelType_Gvsp_Mono8:
        return "mono8";
      case PixelType_Gvsp_Mono10:
        return "mono10";
      case PixelType_Gvsp_Mono10_Packed:
        return "mono10";
      case PixelType_Gvsp_Mono12:
        return "mono12";
      case PixelType_Gvsp_Mono12_Packed:
        return "mono12";
      case PixelType_Gvsp_Mono16:
        return "mono16";
      case PixelType_Gvsp_RGB8_Packed:
        return "rgb8";
      case PixelType_Gvsp_BGR8_Packed:
        return "bgr8";
      case PixelType_Gvsp_RGBA8_Packed:
        return "rgba8";
      case PixelType_Gvsp_BGRA8_Packed:
        return "bgra8";
      case PixelType_Gvsp_YCBCR422_8:
        return "yuv422";
      case PixelType_Gvsp_YUV422_Packed:
        return "yuv422_yuy2";
      case PixelType_Gvsp_BayerRG8:
        return "bayer_rggb8";
      case PixelType_Gvsp_BayerGB8:
        return "bayer_grbg8";
      case PixelType_Gvsp_BayerGR8:
        return "bayer_gbrg8";
      case PixelType_Gvsp_BayerBG8:
        return "bayer_bggr8";
      case PixelType_Gvsp_BayerGB10:
        return "bayer_grbg10";
      case PixelType_Gvsp_BayerGB12:
        return "bayer_grbg12";
      case PixelType_Gvsp_BayerGB12_Packed:
        return "bayer_grbg12";
      case PixelType_Gvsp_RGB10_Packed:
        return "rgb10";
      case PixelType_Gvsp_RGB12_Packed:
        return "rgb12";
      default:
        RCLCPP_WARN(rclcpp::get_logger("camera"), "Unknown pixel type: %ld, defaulting to mono8", pixelType);
        return "mono8";
    }
  }

  // 计算每行字节数的函数
  static uint32_t calculateStep(MvGvspPixelType pixelType, uint32_t width) {
    switch (pixelType) {
      case PixelType_Gvsp_Mono8:
      case PixelType_Gvsp_BayerRG8:
      case PixelType_Gvsp_BayerGB8:
      case PixelType_Gvsp_BayerGR8:
      case PixelType_Gvsp_BayerBG8:
        return width;
      case PixelType_Gvsp_Mono10:
      case PixelType_Gvsp_Mono12:
      case PixelType_Gvsp_Mono16:
        return width * 2;
      case PixelType_Gvsp_Mono10_Packed:
      case PixelType_Gvsp_Mono12_Packed:
        return (width * 10 + 7) / 8 * 2;  // 10位或12位打包格式
      case PixelType_Gvsp_RGB8_Packed:
      case PixelType_Gvsp_BGR8_Packed:
        return width * 3;
      case PixelType_Gvsp_RGBA8_Packed:
      case PixelType_Gvsp_BGRA8_Packed:
        return width * 4;
      case PixelType_Gvsp_YCBCR422_8:
      case PixelType_Gvsp_YUV422_Packed:
        return width * 2;
      case PixelType_Gvsp_BayerGB10:
        return width * 2;
      case PixelType_Gvsp_BayerGB12:
        return width * 2;
      case PixelType_Gvsp_BayerGB12_Packed:
        return (width * 12 + 7) / 8 * 2;  // 12位Bayer打包格式
      case PixelType_Gvsp_RGB10_Packed:
        return width * 4;  // RGB10通常是32位
      case PixelType_Gvsp_RGB12_Packed:
        return width * 6;  // RGB12通常是48位
      default:
        return width;  // 默认单通道
    }
  }



  static void __stdcall ImageCallback(unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser)
  {
    MyNode* node = static_cast<MyNode*>(pUser);
    if (pData && pFrameInfo) {
      // 发布图像
      auto image_msg = sensor_msgs::msg::Image();
      image_msg.header.stamp = node->now();
      image_msg.header.frame_id = "camera";
      image_msg.height = pFrameInfo->nHeight;
      image_msg.width = pFrameInfo->nWidth;
      
      // 根据实际像素格式设置encoding
      image_msg.encoding = pixelTypeToEncoding(pFrameInfo->enPixelType);
      
      // 根据像素格式计算step
      image_msg.step = calculateStep(pFrameInfo->enPixelType, pFrameInfo->nWidth);
      
      image_msg.is_bigendian = 0;
      image_msg.data.resize(pFrameInfo->nFrameLen);
      std::memcpy(image_msg.data.data(), pData, pFrameInfo->nFrameLen);
      node->image_publisher_->publish(image_msg);
    }
  }




  void reconnect()
  {
    // 如果已经在重连中，直接返回
    if (is_reconnecting_) {
      return;
    }
    
    is_reconnecting_ = true;
    RCLCPP_WARN(this->get_logger(), "Starting reconnection thread...");
    
    // 在后台线程中持续尝试重连
    std::thread([this]() {
      while (rclcpp::ok() && is_reconnecting_) {
        // 关闭和销毁当前句柄
        if (handle_) {
          if (is_grabbing_) {
            MV_CC_StopGrabbing(handle_);
            is_grabbing_ = false;
          }
          MV_CC_RegisterExceptionCallBack(handle_, NULL, this);
          MV_CC_RegisterImageCallBackEx(handle_, NULL, this);
          MV_CC_CloseDevice(handle_);
          MV_CC_DestroyHandle(handle_);
          handle_ = nullptr;
        }

        // 重新枚举设备
        MV_CC_DEVICE_INFO_LIST stDeviceList;
        memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
        int enum_result = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
        
        if (enum_result == MV_OK && stDeviceList.nDeviceNum > 0) {
          RCLCPP_INFO(this->get_logger(), "Found %d device(s), attempting to connect...", stDeviceList.nDeviceNum);
          
          // 创建句柄
          int create_result = MV_CC_CreateHandle(&handle_, stDeviceList.pDeviceInfo[0]);
          if (create_result == MV_OK) {
            // 打开设备
            int open_result = MV_CC_OpenDevice(handle_);
            if (open_result == MV_OK) {
              RCLCPP_INFO(this->get_logger(), "Device reconnected successfully!");
              
              // 重新配置相机参数
              reconfigureCamera();
              
              // 注册回调
              MV_CC_RegisterExceptionCallBack(handle_, ExceptionCallBack, this);
              int callback_result = MV_CC_RegisterImageCallBackEx(handle_, ImageCallback, this);
              if (callback_result == MV_OK) {
                // 开始取流
                int start_result = MV_CC_StartGrabbing(handle_);
                if (start_result == MV_OK) {
                  is_grabbing_ = true;
                  is_reconnecting_ = false;  // 重连成功，停止重连线程
                  RCLCPP_INFO(this->get_logger(), "Reconnection completed successfully!");
                  return;
                } else {
                  RCLCPP_ERROR(this->get_logger(), "Failed to restart grabbing: %d", start_result);
                }
              } else {
                RCLCPP_ERROR(this->get_logger(), "Failed to register image callback: %d", callback_result);
              }
              
              // 如果配置或启动失败，关闭设备并继续重试
              MV_CC_CloseDevice(handle_);
              MV_CC_DestroyHandle(handle_);
              handle_ = nullptr;
            } else {
              RCLCPP_WARN(this->get_logger(), "Failed to reopen device: %d", open_result);
            }
          } else {
            RCLCPP_WARN(this->get_logger(), "Failed to recreate handle: %d", create_result);
          }
        } else {
          RCLCPP_WARN(this->get_logger(), "No devices found, will retry...");
        }
        
        // 等待2秒后重试
        std::this_thread::sleep_for(std::chrono::seconds(2));
      }
    }).detach();  // 分离线程，让它在后台运行
  }




  void reconfigureCamera()
  {
    if (!handle_) return;
    
    // 检查设备类型，设置包大小（GigE）
    MV_CC_DEVICE_INFO_LIST stDeviceList;
    memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
    
    if (stDeviceList.nDeviceNum > 0 && stDeviceList.pDeviceInfo[0]->nTLayerType == MV_GIGE_DEVICE) {
      int nPacketSize = MV_CC_GetOptimalPacketSize(handle_);
      if (nPacketSize > 0) {
        MV_CC_SetIntValueEx(handle_, "GevSCPSPacketSize", nPacketSize);
      }
    }

    // 设置触发模式为off
    MV_CC_SetEnumValue(handle_, "TriggerMode", 0);

    // 重新应用当前参数
    double frame_rate = this->get_parameter("frame_rate").as_double();
    MV_CC_SetBoolValue(handle_, "AcquisitionFrameRateEnable", true);
    MV_CC_SetFloatValue(handle_, "AcquisitionFrameRate", frame_rate);

    double exposure_time = this->get_parameter("exposure_time").as_double();
    MV_CC_SetFloatValue(handle_, "ExposureTime", exposure_time);

    double gain = this->get_parameter("gain").as_double();
    MV_CC_SetFloatValue(handle_, "Gain", gain);

    // 像素格式设置（重连时不需要停止采集，因为此时还没有开始采集）
    std::string pixel_format = this->get_parameter("pixel_format").as_string();
    MV_CC_SetEnumValueByString(handle_, "PixelFormat", pixel_format.c_str());

    // 设置缓存节点个数
    MV_CC_SetImageNodeNum(handle_, 10);
  }

  void fps_monitor_callback()
  {
    if (handle_ && is_grabbing_) {
      MVCC_FLOATVALUE stFloatValue;
      memset(&stFloatValue, 0, sizeof(MVCC_FLOATVALUE));
      int result = MV_CC_GetFloatValue(handle_, "ResultingFrameRate", &stFloatValue);
      if (result == MV_OK) {
        RCLCPP_INFO(this->get_logger(), "=== CAMERA SDK FPS: %.2f ===", stFloatValue.fCurValue);
      } else {
        RCLCPP_WARN(this->get_logger(), "Failed to get ResultingFrameRate: %d", result);
      }
    }
  }





  static void __stdcall ExceptionCallBack(unsigned int nMsgType, void* pUser)
  {
    MyNode* node = static_cast<MyNode*>(pUser);
    if (nMsgType == MV_EXCEPTION_DEV_DISCONNECT) {
      RCLCPP_WARN(node->get_logger(), "Device disconnected, attempting to reconnect...");
      node->reconnect();
    }
  }





  MyNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions()) : rclcpp::Node("my_node", options), handle_(nullptr), is_grabbing_(false), is_reconnecting_(false), payload_size_(0), frame_count_(0), last_fps_time_(this->now())
  {
    // 创建图像发布者
    image_publisher_ = this->create_publisher<sensor_msgs::msg::Image>("image_raw", 100);

    // 创建FPS监控定时器，每秒输出一次相机SDK报告的帧率
    fps_monitor_timer_ = this->create_wall_timer(
      std::chrono::seconds(1),
      std::bind(&MyNode::fps_monitor_callback, this));




    // 声明参数
    this->declare_parameter("frame_rate", 1000.0);  // 默认1000fps
    this->declare_parameter("exposure_time", 10000.0);  // 默认10000us
    this->declare_parameter("gain", 0.0);  // 默认0dB
    this->declare_parameter("pixel_format", std::string("Mono8"));  // 默认Mono8




    // 添加参数回调
    auto param_callback = [this](const std::vector<rclcpp::Parameter> &parameters) -> rcl_interfaces::msg::SetParametersResult {
      RCLCPP_INFO(this->get_logger(), "Parameter callback triggered");
      for (const auto &param : parameters) {
        RCLCPP_INFO(this->get_logger(), "Parameter changed: %s", param.get_name().c_str());
        if (param.get_name() == "frame_rate") {
          double frame_rate = param.as_double();
          RCLCPP_INFO(this->get_logger(), "New frame rate: %.2f", frame_rate);
          if (handle_) {
            // 启用帧率控制
            int enable_result = MV_CC_SetBoolValue(handle_, "AcquisitionFrameRateEnable", true);
            if (enable_result != MV_OK) {
              RCLCPP_WARN(this->get_logger(), "Failed to enable frame rate control: %d", enable_result);
            }

            int result = MV_CC_SetFloatValue(handle_, "AcquisitionFrameRate", frame_rate);
            if (result == MV_OK) {
              RCLCPP_INFO(this->get_logger(), "Frame rate updated to: %.2f", frame_rate);
            } else {
              RCLCPP_ERROR(this->get_logger(), "Failed to set frame rate: %d", result);
            }
          } else {
            RCLCPP_WARN(this->get_logger(), "Camera handle not available");
          }

        } else if (param.get_name() == "exposure_time") {
          double exposure_time = param.as_double();
          RCLCPP_INFO(this->get_logger(), "New exposure time: %.2f", exposure_time);
          if (handle_) {
            int result = MV_CC_SetFloatValue(handle_, "ExposureTime", exposure_time);
            if (result == MV_OK) {
              RCLCPP_INFO(this->get_logger(), "Exposure time updated to: %.2f", exposure_time);
            } else {
              RCLCPP_ERROR(this->get_logger(), "Failed to set exposure time: %d", result);
            }
          } else {
            RCLCPP_WARN(this->get_logger(), "Camera handle not available");
          }

        } else if (param.get_name() == "gain") {
          double gain = param.as_double();
          RCLCPP_INFO(this->get_logger(), "New gain: %.2f", gain);
          if (handle_) {
            int result = MV_CC_SetFloatValue(handle_, "Gain", gain);
            if (result == MV_OK) {
              RCLCPP_INFO(this->get_logger(), "Gain updated to: %.2f", gain);
            } else {
              RCLCPP_ERROR(this->get_logger(), "Failed to set gain: %d", result);
            }
          } else {
            RCLCPP_WARN(this->get_logger(), "Camera handle not available");
          }

        } else if (param.get_name() == "pixel_format") {
          std::string pixel_format = param.as_string();
          RCLCPP_INFO(this->get_logger(), "New pixel format: %s", pixel_format.c_str());
          if (handle_) {
            // 像素格式需要停止采集后才能设置
            bool was_grabbing = is_grabbing_;
            if (was_grabbing) {
              MV_CC_StopGrabbing(handle_);
              is_grabbing_ = false;
              RCLCPP_INFO(this->get_logger(), "Stopped grabbing to change pixel format");
            }

            int result = MV_CC_SetEnumValueByString(handle_, "PixelFormat", pixel_format.c_str());
            if (result == MV_OK) {
              RCLCPP_INFO(this->get_logger(), "Pixel format updated to: %s", pixel_format.c_str());
              
              // 如果之前在采集，需要重新开始
              if (was_grabbing) {
                int start_result = MV_CC_StartGrabbing(handle_);
                if (start_result == MV_OK) {
                  is_grabbing_ = true;
                  RCLCPP_INFO(this->get_logger(), "Restarted grabbing with new pixel format");
                } else {
                  RCLCPP_ERROR(this->get_logger(), "Failed to restart grabbing after pixel format change: %d", start_result);
                }
              }
            } else {
              RCLCPP_ERROR(this->get_logger(), "Failed to set pixel format: %d", result);
              
              // 如果设置失败，尝试恢复之前的采集状态
              if (was_grabbing) {
                int start_result = MV_CC_StartGrabbing(handle_);
                if (start_result == MV_OK) {
                  is_grabbing_ = true;
                  RCLCPP_INFO(this->get_logger(), "Restored grabbing after pixel format change failed");
                } else {
                  RCLCPP_ERROR(this->get_logger(), "Failed to restore grabbing: %d", start_result);
                }
              }
            }
          } else {
            RCLCPP_WARN(this->get_logger(), "Camera handle not available");
          }
        }
      }
      rcl_interfaces::msg::SetParametersResult result;
      result.successful = true;
      return result;
    };
    param_callback_handle_ = this->add_on_set_parameters_callback(param_callback);





    // 节点初始化代码
    unsigned int version = MV_CC_GetSDKVersion();
    RCLCPP_INFO(this->get_logger(), "SDK Version: %u", version);

    int init_result = MV_CC_Initialize();
    if (init_result == MV_OK) {
      RCLCPP_INFO(this->get_logger(), "SDK Initialized successfully!");

      // 枚举设备
      MV_CC_DEVICE_INFO_LIST stDeviceList;
      memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
      int enum_result = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
      if (enum_result == MV_OK) {
        RCLCPP_INFO(this->get_logger(), "Enumerated %d devices", stDeviceList.nDeviceNum);
        if (stDeviceList.nDeviceNum > 0) {
          // 创建句柄
          int create_result = MV_CC_CreateHandle(&handle_, stDeviceList.pDeviceInfo[0]);
          if (create_result == MV_OK) {
            RCLCPP_INFO(this->get_logger(), "Handle created successfully!");
            // 打开设备
            int open_result = MV_CC_OpenDevice(handle_);
            if (open_result == MV_OK) {
              RCLCPP_INFO(this->get_logger(), "Device opened successfully!");

              // 检查设备类型，设置包大小（GigE）
              if (stDeviceList.pDeviceInfo[0]->nTLayerType == MV_GIGE_DEVICE) {
                int nPacketSize = MV_CC_GetOptimalPacketSize(handle_);
                if (nPacketSize > 0) {
                  int set_result = MV_CC_SetIntValueEx(handle_, "GevSCPSPacketSize", nPacketSize);
                  if (set_result != MV_OK) {
                    RCLCPP_WARN(this->get_logger(), "Set Packet Size fail: %d", set_result);
                  }
                } else {
                  RCLCPP_WARN(this->get_logger(), "Get Packet Size fail: %d", nPacketSize);
                }
              }

              // 设置触发模式为off
              int trigger_result = MV_CC_SetEnumValue(handle_, "TriggerMode", 0);
              if (trigger_result != MV_OK) {
                RCLCPP_ERROR(this->get_logger(), "Set Trigger Mode fail: %d", trigger_result);
              }

              // 设置帧率从参数读取
              double frame_rate = this->get_parameter("frame_rate").as_double();
              RCLCPP_INFO(this->get_logger(), "Setting frame rate to: %.2f", frame_rate);

              // 启用帧率控制
              int enable_result = MV_CC_SetBoolValue(handle_, "AcquisitionFrameRateEnable", true);
              if (enable_result == MV_OK) {
                RCLCPP_INFO(this->get_logger(), "Frame rate control enabled");
              } else {
                RCLCPP_WARN(this->get_logger(), "Failed to enable frame rate control: %d", enable_result);
              }

              int fps_set_result = MV_CC_SetFloatValue(handle_, "AcquisitionFrameRate", frame_rate);
              if (fps_set_result == MV_OK) {
                RCLCPP_INFO(this->get_logger(), "Frame rate set successfully: %.2f", frame_rate);
              } else {
                RCLCPP_WARN(this->get_logger(), "Set frame rate fail: %d", fps_set_result);
              }

              // 设置曝光时间
              double exposure_time = this->get_parameter("exposure_time").as_double();
              RCLCPP_INFO(this->get_logger(), "Setting exposure time to: %.2f", exposure_time);
              int exp_set_result = MV_CC_SetFloatValue(handle_, "ExposureTime", exposure_time);
              if (exp_set_result == MV_OK) {
                RCLCPP_INFO(this->get_logger(), "Exposure time set successfully: %.2f", exposure_time);
              } else {
                RCLCPP_WARN(this->get_logger(), "Set exposure time fail: %d", exp_set_result);
              }

              // 设置增益
              double gain = this->get_parameter("gain").as_double();
              RCLCPP_INFO(this->get_logger(), "Setting gain to: %.2f", gain);
              int gain_set_result = MV_CC_SetFloatValue(handle_, "Gain", gain);
              if (gain_set_result == MV_OK) {
                RCLCPP_INFO(this->get_logger(), "Gain set successfully: %.2f", gain);
              } else {
                RCLCPP_WARN(this->get_logger(), "Set gain fail: %d", gain_set_result);
              }

              // 设置像素格式
              std::string pixel_format = this->get_parameter("pixel_format").as_string();
              RCLCPP_INFO(this->get_logger(), "Setting pixel format to: %s", pixel_format.c_str());
              int pixel_set_result = MV_CC_SetEnumValueByString(handle_, "PixelFormat", pixel_format.c_str());
              if (pixel_set_result == MV_OK) {
                RCLCPP_INFO(this->get_logger(), "Pixel format set successfully: %s", pixel_format.c_str());
              } else {
                RCLCPP_WARN(this->get_logger(), "Set pixel format fail: %d", pixel_set_result);
              }

              // 设置缓存节点个数
              int node_result = MV_CC_SetImageNodeNum(handle_, 10);
              if (node_result != MV_OK) {
                RCLCPP_WARN(this->get_logger(), "Set Image Node Num fail: %d", node_result);
              }

              // 注册图像回调
              int callback_result = MV_CC_RegisterImageCallBackEx(handle_, ImageCallback, this);
              if (callback_result == MV_OK) {
                RCLCPP_INFO(this->get_logger(), "Image callback registered");
              } else {
                RCLCPP_ERROR(this->get_logger(), "Failed to register image callback: %d", callback_result);
              }

              // 开始取流
              int start_result = MV_CC_StartGrabbing(handle_);
              if (start_result == MV_OK) {
                RCLCPP_INFO(this->get_logger(), "Started grabbing");
                is_grabbing_ = true;
              } else {
                RCLCPP_ERROR(this->get_logger(), "Start Grabbing fail: %d", start_result);
              }

              // 注册异常回调
              int reg_result = MV_CC_RegisterExceptionCallBack(handle_, ExceptionCallBack, this);
              if (reg_result == MV_OK) {
                RCLCPP_INFO(this->get_logger(), "Exception callback registered");
              } else {
                RCLCPP_ERROR(this->get_logger(), "Failed to register exception callback: %d", reg_result);
              }
            } else {
              RCLCPP_ERROR(this->get_logger(), "Failed to open device: %d", open_result);
            }
          } else {
            RCLCPP_ERROR(this->get_logger(), "Failed to create handle: %d", create_result);
          }
        } else {
          RCLCPP_WARN(this->get_logger(), "No devices found");
        }
      } else {
        RCLCPP_ERROR(this->get_logger(), "Failed to enumerate devices: %d", enum_result);
      }
    } else {
      RCLCPP_ERROR(this->get_logger(), "SDK Initialization failed with code: %d", init_result);
    }
  }



  ~MyNode()  // 析构函数,释放资源
  {
    is_reconnecting_ = false;  // 停止重连线程
    
    if (is_grabbing_) {
      is_grabbing_ = false;
      MV_CC_StopGrabbing(handle_);
      RCLCPP_INFO(this->get_logger(), "Stopped grabbing");
    }
    if (handle_) {
      MV_CC_RegisterExceptionCallBack(handle_, NULL, this);  // 注销回调
      MV_CC_RegisterImageCallBackEx(handle_, NULL, this);  // 注销图像回调
      MV_CC_CloseDevice(handle_);
      MV_CC_DestroyHandle(handle_);
      RCLCPP_INFO(this->get_logger(), "Device closed and handle destroyed");
    }
    MV_CC_Finalize();
  }

private:
  void* handle_;
  bool is_grabbing_;
  bool is_reconnecting_;
  unsigned int payload_size_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_publisher_;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr param_callback_handle_;
  rclcpp::TimerBase::SharedPtr fps_monitor_timer_;
  int frame_count_;
  rclcpp::Time last_fps_time_;
};

RCLCPP_COMPONENTS_REGISTER_NODE(MyNode)