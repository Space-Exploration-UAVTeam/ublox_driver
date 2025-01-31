/**
 * This file is part of ublox-driver.
 *
 * Copyright (C) 2021 Aerial Robotics Group, Hong Kong University of Science and Technology
 * Author: CAO Shaozu (shaozu.cao@gmail.com)
 *
 * ublox-driver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ublox-driver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ublox-driver. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <unistd.h>
#include <atomic>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/nav_sat_fix.hpp>
#include <gnss_comm/gnss_utility.hpp>
#include <gnss_comm/gnss_ros.hpp>

constexpr int UTC_OFFSET = 8;

using namespace gnss_comm;

static std::atomic<bool> done(false);

void set_datetime(const gtime_t t, const int timezone)
{
    gtime_t local_t = time_add(t, timezone * 3600);
    gtime_t gtime_utc = gpst2utc(local_t);
    std::vector<double> utc_t;
    utc_t.resize(6);
    utc_t.clear();
    time2epoch(gtime_utc, &(utc_t[0]));
    std::cout << "set system time to: ";
    for (size_t i = 0; i < utc_t.size(); ++i)
        std::cout << utc_t[i] << " ";
    std::cout << std::endl;
    char buf[100];
    sprintf(buf, "%4d-%02d-%02d,%02d:%02d:%02d", int(utc_t[0]), int(utc_t[1]),
            int(utc_t[2]), int(utc_t[3]), int(utc_t[4]), int(utc_t[5]));
    struct tm s_tm;
    strptime(buf, "%Y-%m-%d,%H:%M:%S", &s_tm);
    struct timespec s_timespec;
    s_timespec.tv_sec = mktime(&s_tm);
    s_timespec.tv_nsec = static_cast<long>(t.sec * 1e9 + 0.5);
    // LOG(INFO) << "tv_sec is " << s_timespec.tv_sec << ", and nsec is " << s_timespec.tv_nsec;
    if (clock_settime(CLOCK_REALTIME, &s_timespec) == -1)
        std::cerr << "clock setting error: " << strerror(errno);
}

void receiver_lla_callback(const sensor_msgs::msg::NavSatFix::ConstPtr &lla_msg)
{
    double t_time = static_cast<double>(lla_msg->header.stamp.sec + static_cast<double>(lla_msg->header.stamp.nanosec) * 1e-9) ;
    const gtime_t time = sec2time(t_time);
    if (time.time != 0)
    {
        set_datetime(time, UTC_OFFSET);
        done = true;
    }
}

int main(int argc, char **argv)
{
    // ros::init(argc, argv, "set_system_time");
    rclcpp::init(argc, argv);
    // ros::NodeHandle n("~");
    rclcpp::NodeOptions node_options;
    node_options.automatically_declare_parameters_from_overrides(true);
    auto nh = rclcpp::Node::make_shared("set_system_time", node_options);
    auto sub_lla = nh->create_subscription<sensor_msgs::msg::NavSatFix>("/ublox_driver/receiver_lla", 10, receiver_lla_callback);

    // ros::Rate rate(1);
    // while(!done)
    // {
    //     ros::spinOnce();
    //     rate.sleep();
    // }

    rclcpp::Rate loop(1);
    while (!done)
    {
        rclcpp::spin_some(nh);
        loop.sleep();
    }

    std::cout << "System time updated" << std::endl;
    return 0;
}
