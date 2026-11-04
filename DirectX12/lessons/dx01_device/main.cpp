#include "dx12_course.hpp"
// 本课重点：创建 DXGI adapter、D3D12 device 和队列，并读取调试层消息。
// 入口按课程编号选择路径；实现见 ../../common/dx12_course.hpp。
int main(int argc, char** argv) try {
    return course::dxMain(argc, argv, 1);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
