#include "dx12_course.hpp"
// 本课重点：绑定索引缓冲和帧 CBV，区分帧数据与 root constants。
// 入口按课程编号选择路径；实现见 ../../common/dx12_course.hpp。
int main(int argc, char** argv) try {
    return course::dxMain(argc, argv, 5);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
