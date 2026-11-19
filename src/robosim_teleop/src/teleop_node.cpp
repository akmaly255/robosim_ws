#include <memory>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <functional>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "sensor_msgs/msg/joy.hpp"

#include <trajectory_msgs/msg/joint_trajectory.hpp>
#include <trajectory_msgs/msg/joint_trajectory_point.hpp>

class TeleopNode : public rclcpp::Node 
{
public:
  TeleopNode() : Node("TeleopNode")
  {

    // Parametes
    this->declare_parameter("linear_axis", 1);
    this->declare_parameter("angular_axis", 0);

    this->declare_parameter("max_linear_speed", 1.0);
    this->declare_parameter("max_angular_speed", 2.0);

    this->declare_parameter("pan_axis", 3);
    this->declare_parameter("tilt_axis", 4);

    this->declare_parameter("max_pan_speed", 2.0);
    this->declare_parameter("max_tilt_speed", 2.0);

    this->declare_parameter("camera_forward_button", 1);

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

    pan_tilt_timer_ = this->create_wall_timer(
      std::chrono::milliseconds(50),
      std::bind(&TeleopNode::publish_pan_tilt, this)
    );
  }

private:
  void joy_callback(const sensor_msgs::msg::Joy &msg)
  {
    linear_axis_ = this->get_parameter("linear_axis").as_int();
    angular_axis_ = this->get_parameter("angular_axis").as_int();

    camera_forward_button_ = this->get_parameter("camera_forward_button").as_int();

    pan_axis_ = this->get_parameter("pan_axis").as_int();
    tilt_axis_ = this->get_parameter("tilt_axis").as_int();

    max_linear_speed_ = this->get_parameter("max_linear_speed").as_double();
    max_angular_speed_ = this->get_parameter("max_angular_speed").as_double();

    auto linear_input = msg.axes[linear_axis_];
    auto angular_input = msg.axes[angular_axis_];

    if(pan_axis_ >= 0 && tilt_axis_ >= 0 && 
      static_cast<size_t>(pan_axis_) < msg.axes.size() && 
      static_cast<size_t>(tilt_axis_) < msg.axes.size())
    {
      pan_input_ = msg.axes[pan_axis_];
      tilt_input_ = msg.axes[tilt_axis_];

      last_joy_time_ = std::chrono::steady_clock::now();
    }else{
      pan_input_ = 0.0;
      tilt_input_ = 0.0;
    }

    camera_forward_input_ = msg.buttons[camera_forward_button_];

    RCLCPP_INFO(
    this->get_logger(),
    "\nlinear axis : %f, angular axis : %f", 
    linear_input, angular_input);

    auto cmd = geometry_msgs::msg::Twist();

    cmd.linear.x = linear_input * max_linear_speed_;
    cmd.angular.z = angular_input * max_angular_speed_;

    cmd_publisher_->publish(cmd);
  }
  void publish_pan_tilt()
  {
    max_pan_speed_ = this->get_parameter("max_pan_speed").as_double();
    max_tilt_speed_ = this->get_parameter("max_tilt_speed").as_double();

    constexpr double dt = 0.05;
    constexpr double angle_limit = 3.1416;
    constexpr double deadzone = 0.05;

    const bool fresh_input = 
      last_joy_time_.time_since_epoch().count() != 0 &&
      std::chrono::duration<double>(
        std::chrono::steady_clock::now() - last_joy_time_
      ).count() < 0.5;

    double pan = fresh_input ? pan_input_ : 0.0;
    double tilt = fresh_input ? tilt_input_ : 0.0;

    if(std::abs(pan) < deadzone){
      pan = 0.0;
    }

    if(std::abs(tilt) < deadzone){
      tilt = 0.0;
    }

    pan_angle_ += pan * max_pan_speed_ * dt;
    tilt_angle_ += -tilt * max_tilt_speed_ * dt;

    pan_angle_ = std::clamp(
      pan_angle_, -angle_limit, angle_limit);
    tilt_angle_ = std::clamp(
      tilt_angle_, -1.5708, 0.7);

    if(camera_forward_input_){
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

    point.time_from_start = 
      rclcpp::Duration::from_seconds(0.15);

    trajectory.points.push_back(point);

    pan_tilt_publisher_->publish(trajectory);
  }
  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_subscriber_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_publisher_;

  rclcpp::Publisher<trajectory_msgs::msg::JointTrajectory>::SharedPtr pan_tilt_publisher_;

  rclcpp::TimerBase::SharedPtr pan_tilt_timer_;

  std::chrono::steady_clock::time_point last_joy_time_{};

  int linear_axis_, angular_axis_, pan_axis_, tilt_axis_,
      camera_forward_button_, camera_forward_input_;
  double max_linear_speed_, max_angular_speed_, max_pan_speed_, max_tilt_speed_;
  double pan_input_, tilt_input_, pan_angle_, tilt_angle_;
};

int main(int argc, char *argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<TeleopNode>());
  rclcpp::shutdown();
  
 return 0;
}

