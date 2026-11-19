import os

from launch import LaunchDescription
from launch.actions import (
  DeclareLaunchArgument,
  IncludeLaunchDescription,
  SetEnvironmentVariable
)
from launch.launch_description_sources import (
  PythonLaunchDescriptionSource
)
from launch.substitutions import (
  Command,
  FindExecutable,
  LaunchConfiguration,
  PathJoinSubstitution,
  EnvironmentVariable
)

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.parameter_descriptions import ParameterValue

def generate_launch_description():

  gazebo_ip = SetEnvironmentVariable(
    name='GAZEBO_IP',
    value='127.0.0.1'
  )

  ign_ip = SetEnvironmentVariable(
    name='IGN_IP',
    value='127.0.0.1'
  )

  gz_ip = SetEnvironmentVariable(
    name='GZ_IP',
    value='127.0.0.1'
  )

  default_model_path = 'urdf/robot.urdf.xacro'
  default_world_path = 'worlds/world.sdf'
  default_rviz_path = 'rviz/robot.rviz'

  model = LaunchConfiguration('model')
  world = LaunchConfiguration('world')
  rviz_config = LaunchConfiguration('rviz_config')

  model_path = PathJoinSubstitution([
    FindPackageShare('robosim_description'),
    model
  ])

  world_path = PathJoinSubstitution([
    FindPackageShare('robosim_simulation'),
    world
  ])

  rviz_path = PathJoinSubstitution([
    FindPackageShare('robosim_description'),
    rviz_config
  ])

  models_path = PathJoinSubstitution([
    FindPackageShare('robosim_simulation'),
    'models'
  ])

  gazebo_model_path = SetEnvironmentVariable(
    name='GAZEBO_MODEL_PATH',
    value=[
      models_path,
      os.pathsep,
      EnvironmentVariable(
        'GAZEBO_MODEL_PATH',
        default_value=''
      )
    ]
  )

  robot_description = ParameterValue(
    Command([
      FindExecutable(name='xacro'),
      ' ',
      model_path
    ]),
    value_type=str
  )

  gazebo = IncludeLaunchDescription(
    PythonLaunchDescriptionSource(
      PathJoinSubstitution([
        FindPackageShare('gazebo_ros'),
        'launch',
        'gazebo.launch.py'
      ])
    ),
    launch_arguments={
      'world': world_path,
      'verbose': 'true'
    }.items()
  )

  robot_state_publisher = Node (
    package='robot_state_publisher',
    executable='robot_state_publisher',
    output='screen',
    parameters=[{
      'robot_description': robot_description,
      'use_sim_time': True
    }]
  )

  spawn_robot = Node(
    package='gazebo_ros',
    executable='spawn_entity.py',
    output='screen',
    arguments=[
      '-entity', 'mobile_robot',
      '-topic', 'robot_description',
      '-x', '1.0',
      '-y', '0.0',
      '-z', '1.10'
    ]
  )

  joint_state_broadcaster_spawner = Node(
    package='controller_manager',
    executable='spawner',
    output='screen',
    arguments=[
      'joint_state_broadcaster',
      '--controller-manager', '/controller_manager',
      '--controller-manager-timeout', '120'
    ]
  )

  pan_tilt_controller_spawner = Node(
    package='controller_manager',
    executable='spawner',
    output='screen',
    arguments=[
      'pan_tilt_controller',
      '--controller-manager', '/controller_manager',
      '--controller-manager-timeout', '120'
    ]
  )

  rviz = Node(
    package='rviz2',
    executable='rviz2',
    name='rviz2',
    output='screen',
    arguments=['-d', rviz_path],
    parameters=[{'use_sim_time': True}]
  )

  return LaunchDescription([
    DeclareLaunchArgument(
      'model',
      default_value=default_model_path,
      description='Path to robot xacro file'
    ),
    DeclareLaunchArgument(
      'world',
      default_value=default_world_path,
      description='Path to Gazebo world file'
    ),
    DeclareLaunchArgument(
      'rviz_config',
      default_value=default_rviz_path,
      description='Path to Gazebo world file'
    ),

    gazebo_ip,
    ign_ip,
    gz_ip,

    gazebo_model_path,

    gazebo,
    robot_state_publisher,
    spawn_robot,
    joint_state_broadcaster_spawner,
    pan_tilt_controller_spawner,
    rviz
  ])

