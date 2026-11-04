#include "native_app.hpp"
// 本课重点：创建 surface/交换链，观察 acquire、清屏、submit、present 的顺序。
// 入口按课程编号选择路径；实现见 ../../common/native_app.hpp 和 ../../../common/scene_vk.hpp。
int main(int argc, char** argv) try {
    return course::nativeMain(argc, argv, 2);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
