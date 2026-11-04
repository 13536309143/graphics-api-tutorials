#include "dx12_course.hpp"
// 本课重点：从离屏 RENDER_TARGET 拷贝到交换链，随后恢复资源状态并呈现。
// 入口按课程编号选择路径；实现见 ../../common/dx12_course.hpp。
int main(int argc, char** argv) try {
    return course::dxMain(argc, argv, 10);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
