from launch import LaunchDescription

from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource

from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():

  simulation_launch = IncludeLaunchDescription( 
    PythonLaunchDescriptionSource(
      PathJoinSubstitution([
        FindPackageShare('robosim_simulation'),
        'launch',
        'simulation.launch.py'
      ])
    )
  )

  teleop_launch = IncludeLaunchDescription( 
    PythonLaunchDescriptionSource(
      PathJoinSubstitution([
        FindPackageShare('robosim_teleop'),
        'launch',
        'teleop.launch.py'
      ])
    )
  )

  return LaunchDescription([
    simulation_launch,

    teleop_launch
  ])
