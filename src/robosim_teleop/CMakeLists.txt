cmake_minimum_required(VERSION 3.8)
project(robosim_teleop)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(trajectory_msgs REQUIRED)

add_executable(teleop_node src/teleop_node.cpp)
ament_target_dependencies(teleop_node rclcpp geometry_msgs sensor_msgs trajectory_msgs)

target_compile_features(teleop_node PUBLIC cxx_std_17)

install (DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME})

install(TARGETS
  teleop_node
  DESTINATION lib/${PROJECT_NAME})

ament_package()
