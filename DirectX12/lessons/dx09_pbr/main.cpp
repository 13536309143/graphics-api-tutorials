#include "dx12_course.hpp"
// 本课重点：用每绘制 root constants 改变金属度和粗糙度，计算直接光 PBR。
// 入口按课程编号选择路径；实现见 ../../common/dx12_course.hpp。
int main(int argc, char** argv) try {
    return course::dxMain(argc, argv, 9);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
