#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "rysen_apexhand/choreography_node.hpp"

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);

    // 实例化节点
    auto node = std::make_shared<rysen_apexhand::ChoreographyNode>(rclcpp::NodeOptions());

    // 开始自旋，处理定时器和回调
    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}