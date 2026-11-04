#include "nvapp_course.hpp"
// 本课重点：用 RenderTarget 显示直接光 PBR，并通过 ImGui 调整粗糙度。
// 入口按课程编号选择路径；实现见 ../../common/nvapp_course.hpp 的生命周期回调。
int main(int argc, char** argv) try {
    return course::nvproMain(argc, argv, 6);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
