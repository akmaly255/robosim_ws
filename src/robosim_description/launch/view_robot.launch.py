from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import (
  Command,
  FindExecutable,
  LaunchConfiguration,
  PathJoinSubstitution
)

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.parameter_descriptions import ParameterValue

def generate_launch_description():
  default_model_path = PathJoinSubstitution([
    FindPackageShare('robosim_description'),
    'urdf',
    'robot.urdf.xacro'
  ])

  rviz_config_path = PathJoinSubstitution([
    FindPackageShare('robosim_description'),
    'rviz',
    'robot.rviz'
  ])

  model = LaunchConfiguration('model')

  robot_description = ParameterValue(
    Command([
      FindExecutable(name='xacro'),
      ' ',
      model
    ]),
    value_type=str
  )

  robot_state_publisher =  Node(
    package='robot_state_publisher',
    executable='robot_state_publisher',
    parameters=[{
      'robot_description': robot_description
    }],
    output='screen'
  )

  joint_state_publisher_gui = Node(
    package='joint_state_publisher_gui',
    executable='joint_state_publisher_gui',
    output='screen'
  )

  rviz = Node(
    package='rviz2',
    executable='rviz2',
    name='rviz2',
    output='screen',
    arguments=[{'-d', rviz_config_path}],
    parameters=[{'use_sim_time': True}]
  )

  return LaunchDescription([
    DeclareLaunchArgument(
      name='model',
      default_value=default_model_path,
      description='Path to robot xacro file'
    ),

    robot_state_publisher,

    joint_state_publisher_gui,

    rviz
  ])
