#include "dx12_course.hpp"
// 本课重点：创建 flip-discard 交换链，完成清屏、状态切换与 Present。
// 入口按课程编号选择路径；实现见 ../../common/dx12_course.hpp。
int main(int argc, char** argv) try {
    return course::dxMain(argc, argv, 2);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
