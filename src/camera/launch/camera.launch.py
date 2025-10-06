from launch import LaunchDescription
from launch.actions import RegisterEventHandler
from launch.event_handlers import OnProcessStart
from launch_ros.actions import Node, ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    rviz_config = get_package_share_directory('camera') + '/rviz/camera.rviz'

    # 1. 定义摄像头容器
    container = ComposableNodeContainer(
        name='camera_container',
        namespace='',
        package='rclcpp_components',
        executable='component_container',
        composable_node_descriptions=[
            ComposableNode(
                package='camera',
                plugin='MyNode',
                name='my_node',
                parameters=[
                    {'frame_rate': 1000.0},
                    {'exposure_time': 10000.0},
                    {'gain': 0.0},
                    {'pixel_format': 'Mono8'}
                ]
            ),
            ComposableNode(
                package='camera',
                plugin='ImageViewerNode',
                name='image_viewer_node'
            ),
        ],
        output='screen',
    )

    # 2. 定义 RViz2 节点
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        arguments=['-d', rviz_config],
        output='screen'
    )

    # 3. 创建一个事件处理器
    # 它会监视 'camera_container' 进程的启动事件
    # 一旦 'camera_container' 启动了，它就会执行 on_start 中定义的动作（即启动rviz_node）
    rviz_starter = RegisterEventHandler(
        event_handler=OnProcessStart(
            target_action=container,
            on_start=[rviz_node]
        )
    )

    return LaunchDescription([
        container,
        # 启动事件处理器，而不是直接启动rviz_node
        rviz_starter
    ])