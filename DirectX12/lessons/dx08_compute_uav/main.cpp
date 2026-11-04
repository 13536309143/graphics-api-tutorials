#include "dx12_course.hpp"
// 本课重点：计算着色器写 UAV，切换到拷贝源后通过 READBACK heap 核对结果。
// 入口按课程编号选择路径；实现见 ../../common/dx12_course.hpp。
int main(int argc, char** argv) try {
    return course::dxMain(argc, argv, 8);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
