#include "nvapp_course.hpp"
// 本课重点：在 IAppElement 的 onRender 中复用底层 Vulkan 三角形渲染器。
// 入口按课程编号选择路径；实现见 ../../common/nvapp_course.hpp 的生命周期回调。
int main(int argc, char** argv) try {
    return course::nvproMain(argc, argv, 3);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
