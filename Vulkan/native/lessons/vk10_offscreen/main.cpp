#include "native_app.hpp"
// 本课重点：先离屏渲染，再 blit 到交换链，跟踪两张图像的布局切换。
// 入口按课程编号选择路径；实现见 ../../common/native_app.hpp 和 ../../../common/scene_vk.hpp。
int main(int argc, char** argv) try {
    return course::nativeMain(argc, argv, 10);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
