#include "native_app.hpp"
// 本课重点：枚举并选择支持 Vulkan 1.3 和所需队列/特性的设备。
// 入口按课程编号选择路径；实现见 ../../common/native_app.hpp 和 ../../../common/scene_vk.hpp。
int main(int argc, char** argv) try {
    return course::nativeMain(argc, argv, 1);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
