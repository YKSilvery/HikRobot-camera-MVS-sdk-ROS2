cmake_minimum_required(VERSION 3.8)
project(camera)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)

# Set paths to third_party SDK
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(MVS_INCLUDE_DIR ${THIRD_PARTY_DIR}/include)
set(MVS_LIB_DIR ${THIRD_PARTY_DIR}/lib/64)

link_directories(${MVS_LIB_DIR})

add_library(test_node SHARED src/test_node.cpp)
rclcpp_components_register_node(test_node PLUGIN "MyNode" EXECUTABLE my_node_exe)

add_library(image_viewer_node SHARED src/image_viewer_node.cpp)
rclcpp_components_register_node(image_viewer_node PLUGIN "ImageViewerNode" EXECUTABLE image_viewer_node_exe)

ament_target_dependencies(test_node rclcpp sensor_msgs rclcpp_components)
target_link_libraries(test_node MvCameraControl)
set_target_properties(test_node PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
target_include_directories(test_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${MVS_INCLUDE_DIR})
target_compile_features(test_node PUBLIC c_std_99 cxx_std_17)  # Require C99 and C++17

ament_target_dependencies(image_viewer_node rclcpp sensor_msgs rclcpp_components)
target_compile_features(image_viewer_node PUBLIC c_std_99 cxx_std_17)

install(TARGETS test_node image_viewer_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME})

# Install launch files
install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
  FILES_MATCHING PATTERN "*.py"
)

# Install RViz config files
install(DIRECTORY rviz/
  DESTINATION share/${PROJECT_NAME}/rviz
  FILES_MATCHING PATTERN "*.rviz"
)

# Install MVS SDK libraries
install(DIRECTORY ${MVS_LIB_DIR}/
  DESTINATION lib
  FILES_MATCHING PATTERN "*.so*"
  PATTERN "ThirdParty" EXCLUDE)

# Install MVS SDK config and cti files
install(DIRECTORY ${MVS_LIB_DIR}/
  DESTINATION lib
  FILES_MATCHING PATTERN "*.ini"
  PATTERN "*.cti"
  PATTERN "ThirdParty" EXCLUDE)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
