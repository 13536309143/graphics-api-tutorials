#include "dx12_course.hpp"
// 本课重点：设置 DSV 与深度状态，用 SV_InstanceID 放置多个立方体。
// 入口按课程编号选择路径；实现见 ../../common/dx12_course.hpp。
int main(int argc, char** argv) try {
    return course::dxMain(argc, argv, 7);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
