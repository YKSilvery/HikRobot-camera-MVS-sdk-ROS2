import launch
import launch_ros.actions
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    # 声明启动参数
    frame_rate_arg = DeclareLaunchArgument(
        'frame_rate',
        default_value='1.0',
        description='相机帧率 (fps)'
    )

    exposure_time_arg = DeclareLaunchArgument(
        'exposure_time',
        default_value='10000.0',
        description='曝光时间 (微秒)'
    )

    gain_arg = DeclareLaunchArgument(
        'gain',
        default_value='0.0',
        description='增益 (dB)'
    )

    pixel_format_arg = DeclareLaunchArgument(
        'pixel_format',
        default_value='Mono8',
        description='像素格式 (Mono8, RGB8, etc.)'
    )

    # 创建相机节点
    camera_node = launch_ros.actions.Node(
        package='camera',
        executable='test_node',
        name='my_node',
        output='screen',
        parameters=[{
            'frame_rate': LaunchConfiguration('frame_rate'),
            'exposure_time': LaunchConfiguration('exposure_time'),
            'gain': LaunchConfiguration('gain'),
            'pixel_format': LaunchConfiguration('pixel_format'),
        }]
    )

    return LaunchDescription([
        frame_rate_arg,
        exposure_time_arg,
        gain_arg,
        pixel_format_arg,
        camera_node,
    ])