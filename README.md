# Robosim: ROS 2 Differential-Drive Robot Simulation

A joystick-teleoperated differential-drive mobile robot simulated in **Gazebo Classic** using **ROS 2 Humble**. The robot includes a two-axis pan–tilt camera gimbal, an RGB camera, odometry, joint-state feedback, and RViz2 visualization. A custom maze-like environment contains reusable obstacle models stored alongside the simulation source.

> **Project status:** Core simulation and integration have been tested on the development computer. Reproduction on a second computer is still pending.
>
> **Before publishing:** Replace the repository URL and bringup launch filename placeholders below, confirm the topic names and joystick mappings, and document the licenses/attribution for downloaded models.

## Features

- Differential-drive robot model built from URDF/Xacro and basic geometries.
- Joystick teleoperation: `sensor_msgs/msg/Joy` → `geometry_msgs/msg/Twist` (`/cmd_vel`).
- Gazebo Classic differential-drive plugin for wheel motion and odometry.
- `gazebo_ros2_control` and a joint trajectory controller for pan/tilt motion.
- Gazebo RGB camera exposed to ROS 2 as `sensor_msgs/msg/Image` and `sensor_msgs/msg/CameraInfo`.
- RViz2 configuration for robot geometry, TF, and camera imagery.
- Maze-like SDF world and local model assets for reusable obstacles.
- A unified ROS 2 bringup package for launching the system.

## Software environment

Developed for:

- Ubuntu **22.04**
- ROS 2 **Humble**
- **Gazebo Classic 11** (not the newer Gazebo / Gazebo Sim)
- `colcon`, `rosdep`, and an input joystick/gamepad

Gazebo Classic is end-of-life; use the same environment for reproducible results, and plan a future migration if long-term maintenance is needed.

### ROS 2 dependencies

ROS 2 dependencies should be declared in the relevant `package.xml` files. Core runtime components include:

- `gazebo_ros`, `gazebo_plugins`, `gazebo_ros2_control`
- `ros2_control`, `controller_manager`, `joint_state_broadcaster`, `joint_trajectory_controller`
- `robot_state_publisher`, `xacro`, `rviz2`, `joy`
- `rclcpp`, `sensor_msgs`, `geometry_msgs`, `trajectory_msgs`

The exact list depends on the current source and manifest files. Check them before tagging a release.

## Workspace layout

```text
robosim_ws/
├── src/
│   ├── robosim_description/   # URDF, Xacro, robot materials, RViz settings
│   ├── robosim_simulation/    # Gazebo worlds, local models, controller config
│   │   ├── worlds/
│   │   ├── models/
│   │   ├── config/
│   │   └── launch/
│   ├── robosim_teleop/       # Joystick teleoperation C++ node
│   └── robosim_bringup/      # Entry point to launch the whole system
├── README.md
└── .gitignore
```

The generated `build/`, `install/`, and `log/` directories are intentionally **not** stored in Git.

## Installation on a new computer

### 1. Install the ROS environment

Install ROS 2 Humble Desktop and compatible Gazebo Classic / ROS integration packages for Ubuntu 22.04. Follow the official ROS 2 Humble Ubuntu installation procedure before proceeding. You will need network access for this initial setup.

```bash
source /opt/ros/humble/setup.bash
sudo apt update
sudo apt install python3-colcon-common-extensions python3-rosdep
```

Initialize `rosdep` only if it has not already been initialized:

```bash
sudo rosdep init
rosdep update
```

If `rosdep init` reports that it was already initialized, simply run `rosdep update`.

### 2. Clone the repository

Replace `<USERNAME>` and `<REPOSITORY>` with the published repository details:

```bash
git clone https://github.com/<USERNAME>/<REPOSITORY>.git ~/robosim_ws
cd ~/robosim_ws
```

### 3. Install declared dependencies

```bash
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src --rosdistro humble -r -y
```

If dependencies are missing, review the affected package's `package.xml` and install any Gazebo Classic-specific packages unavailable through `rosdep`.

### 4. Build and source

```bash
colcon build --symlink-install
source install/setup.bash
```

### 5. Launch the simulation

Find the installed bringup launch filename:

```bash
ls "$(ros2 pkg prefix --share robosim_bringup)/launch"
```

Then run (replace the placeholder with the actual filename):

```bash
ros2 launch robosim_bringup <BRINGUP_LAUNCH_FILE>.launch.py
```

Alternatively, launch the simulation directly if its launch file is installed:

```bash
ros2 launch robosim_simulation simulation.launch.py \
  world:=worlds/world_v1_modular.sdf
```

The modular world requires the local Gazebo model directories to be discoverable through `GAZEBO_MODEL_PATH`. The simulation launch file should set this automatically before `gzserver` is started. Do not rely on previously downloaded models in `~/.gazebo/models`.

## Joystick and camera

Connect a compatible joystick before or after bringup, depending on your launch setup. Verify the input device and ROS 2 messages:

```bash
ls /dev/input/js* 2>/dev/null
ros2 topic echo /joy
ros2 topic echo /cmd_vel
```

The teleop node converts joystick axes to linear/angular velocity and pan/tilt commands. **Document the specific axis mapping, speed limits, dead zones, and stop-on-disconnect behavior here** after verifying the final teleop parameters.

Verify the controllers and camera topics:

```bash
ros2 control list_controllers
ros2 topic list -t | grep -Ei 'camera|image'
ros2 topic hz /camera/image_raw
```

The expected camera image type is `sensor_msgs/msg/Image`, **not** `camera_msgs/msg/Image`. Actual topic names depend on the camera plugin's configuration.

## Reproducibility checklist

On a **different computer** with Ubuntu 22.04 / ROS 2 Humble:

- [ ] Clone the repository into a fresh workspace.
- [ ] Install dependencies using `rosdep` and the documented Gazebo Classic packages.
- [ ] Build successfully with `colcon build --symlink-install`.
- [ ] Bring up Gazebo, robot_state_publisher, controllers, joystick, and RViz2 with one launch command.
- [ ] Confirm the intended world and all obstacle models/meshes/textures load.
- [ ] Confirm differential-drive motion and odometry work.
- [ ] Confirm pan/tilt commands and camera images work.
- [ ] Confirm TF and RViz2 displays work.
- [ ] Confirm clean shutdown without leftover `gzserver` processes.
- [ ] Disconnect internet **after installation** and confirm the entire system can still start and operate.

Capture the exact OS, package versions, launch command, pass/fail result, and any fixes required during this test.

## Troubleshooting

**Missing obstacle models:** Verify that the `models/` directory is installed and registered in `GAZEBO_MODEL_PATH`. Check `model://` references in the world and model mesh URIs. If needed, inspect the environment of the running Gazebo server:

```bash
PID=$(pgrep -n -x gzserver)
tr '\0' '\n' < "/proc/$PID/environ" | grep '^GAZEBO_MODEL_PATH='
```

**Camera visible but no image topic:** Ensure `libgazebo_ros_camera.so` is nested directly inside the camera `<sensor>` element; inspect the generated SDF and Gazebo logs.

**Joystick topics appear but the robot does not move:** Inspect `/joy`, `/cmd_vel`, `/odom`, controller states, and whether Gazebo physics is paused. If disabling Wi-Fi disrupts ROS 2 DDS communication, test a consistent local-only ROS 2 configuration for all involved processes.

**Gazebo restarts at an unexpected object pose:** Check for a saved `<state>` section in the world file and ensure an old `gzserver` process is not still running.

## Third-party assets and licenses

The simulation contains locally copied models initially obtained using Gazebo's Insert tab. **Before distributing this repository publicly, inspect each model's license and include attribution when required.** The repository's own license does not automatically grant redistribution rights for third-party models.

