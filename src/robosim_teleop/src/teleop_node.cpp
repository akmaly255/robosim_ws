#include <memory>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <functional>
#include <cstddef>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include "trajectory_msgs/msg/joint_trajectory.hpp"
#include "trajectory_msgs/msg/joint_trajectory_point.hpp"

class TeleopNode : public rclcpp::Node 
{
public:
  TeleopNode() : Node("TeleopNode")
  {

    // Parametes
    this->declare_parameter<int>("linear_axis", 1);
    this->declare_parameter<int>("angular_axis", 0);

    this->declare_parameter<double>("max_linear_speed", 1.0);
    this->declare_parameter<double>("max_angular_speed", 2.0);

    this->declare_parameter<int>("pan_axis", 3);
    this->declare_parameter<int>("tilt_axis", 4);

    this->declare_parameter<double>("max_pan_speed", 2.0);
    this->declare_parameter<double>("max_tilt_speed", 2.0);

    this->declare_parameter<int>("camera_forward_button", 1);

    this->declare_parameter<double>("input_timeout", 0.3);

    // Create subscriber to /joy
    joy_subscriber_ = this->create_subscription<sensor_msgs::msg::Joy>(
    "joy", 
    10,
    std::bind(&TeleopNode::joy_callback, this, std::placeholders::_1));

    // Create publisher to /cmd_vel
    cmd_publisher_ = this->create_publisher<geometry_msgs::msg::Twist>(
    "cmd_vel",
    10);

    // Create publisher to /pan_tilt_controller/joint_trajectory
    pan_tilt_publisher_ = this->create_publisher<trajectory_msgs::msg::JointTrajectory>(
    "/pan_tilt_controller/joint_trajectory",
    10);

    last_update_time_ = std::chrono::steady_clock::now();

    watchdog_timer_ = this->create_wall_timer(
      std::chrono::milliseconds(50),
      std::bind(&TeleopNode::publish_commands, this)
    );
  }

private:
  void joy_callback(const sensor_msgs::msg::Joy &msg)
  {
    const int linear_axis = this->get_parameter("linear_axis").as_int();
    const int angular_axis = this->get_parameter("angular_axis").as_int();

    const int camera_forward_button = this->get_parameter("camera_forward_button").as_int();

    const int pan_axis = this->get_parameter("pan_axis").as_int();
    const int tilt_axis = this->get_parameter("tilt_axis").as_int();

    auto valid_axis = [&msg](int axis) {
      return axis >= 0 &&
        static_cast<size_t>(axis) < msg.axes.size();
    };

    auto valid_button = [&msg](int button) {
      return button >= 0 &&
        static_cast<size_t>(button) < msg.buttons.size();
    };

    if(!valid_axis(linear_axis) || !valid_axis(angular_axis)) {

      joy_received_ = false;

      linear_input_ = 0.0;
      angular_input_ = 0.0;

      pan_input_ = 0.0;
      tilt_input_ = 0.0;

      camera_forward_input_ = false;

      return;
    }

    linear_input_ = msg.axes[linear_axis];
    angular_input_ = msg.axes[angular_axis];

    pan_input_ =  valid_axis(pan_axis) ? msg.axes[pan_axis] : 0.0;
    tilt_input_ =  valid_axis(tilt_axis) ? msg.axes[tilt_axis] : 0.0;

    camera_forward_input_ = valid_button(camera_forward_button) && 
      msg.buttons[camera_forward_button ] != 0;

    last_joy_time_ = std::chrono::steady_clock::now();

    joy_received_ = true;
  }
  void publish_commands()
  {
    const auto now = std::chrono::steady_clock::now();

    const double dt = std::min(
      std::chrono::duration<double>(
        now - last_update_time_
      ).count(),
      0.1
    );

    last_update_time_ = now;

    // Differential Drive Control
    const bool fresh_input = is_joy_fresh();

    const double max_linear_speed = this->get_parameter("max_linear_speed").as_double();
    const double max_angular_speed = this->get_parameter("max_angular_speed").as_double();

    geometry_msgs::msg::Twist cmd;

    if(fresh_input){
      cmd.linear.x = linear_input_ * max_linear_speed;
      cmd.angular.z = angular_input_ * max_angular_speed;
    }

    cmd_publisher_->publish(cmd);

    // Pan-Tilt Gimbal Control
    const double max_pan_speed = this->get_parameter("max_pan_speed").as_double();
    const double max_tilt_speed = this->get_parameter("max_tilt_speed").as_double();

    constexpr double deadzone = 0.05;

    double pan = fresh_input ? pan_input_ : 0.0;
    double tilt = fresh_input ? tilt_input_ : 0.0;

    if(std::abs(pan) < deadzone)
      pan = 0.0;
    if(std::abs(tilt) < deadzone)
      tilt = 0.0;

    pan_angle_ += pan * max_pan_speed * dt;
    tilt_angle_ += -tilt * max_tilt_speed * dt;

    pan_angle_ = std::clamp(pan_angle_, -3.1416, 3.1416);
    tilt_angle_ = std::clamp(tilt_angle_, -1.5708, 0.7);

    if(fresh_input && camera_forward_input_){
      pan_angle_ = 0.0;
      tilt_angle_ = 0.0;
    }

    trajectory_msgs::msg::JointTrajectory trajectory;

    trajectory.joint_names = {
      "pan_joint",
      "tilt_joint"
    };

    trajectory_msgs::msg::JointTrajectoryPoint point;

    point.positions = {
      pan_angle_,
      tilt_angle_
    };

    point.time_from_start = rclcpp::Duration::from_seconds(0.15);
    trajectory.points.push_back(point);

    pan_tilt_publisher_->publish(trajectory);
  }
  bool is_joy_fresh() const {
    if(!joy_received_) {
      return false;
    }

    const double elapsed_time = 
      std::chrono::duration<double>(
        std::chrono::steady_clock::now() - last_joy_time_
      ).count();

    const double timeout =
      this->get_parameter("input_timeout").as_double();

    return elapsed_time < timeout;
  }

  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_subscriber_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_publisher_;

  rclcpp::Publisher<trajectory_msgs::msg::JointTrajectory>::SharedPtr pan_tilt_publisher_;

  rclcpp::TimerBase::SharedPtr watchdog_timer_;

  std::chrono::steady_clock::time_point last_joy_time_{};
  std::chrono::steady_clock::time_point last_update_time_{};

  bool joy_received_ = false;
  bool camera_forward_input_ = false;

  double linear_input_ = 0.0;
  double angular_input_ = 0.0;

  double pan_input_ = 0.0;
  double tilt_input_ = 0.0;

  double pan_angle_ = 0.0;
  double tilt_angle_ = 0.0;
};

int main(int argc, char *argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<TeleopNode>());
  rclcpp::shutdown();
  
 return 0;
}

