#include "dx12_course.hpp"
// 本课重点：上传顶点并设置 vertex buffer view，核对 input layout 偏移。
// 入口按课程编号选择路径；实现见 ../../common/dx12_course.hpp。
int main(int argc, char** argv) try {
    return course::dxMain(argc, argv, 4);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
