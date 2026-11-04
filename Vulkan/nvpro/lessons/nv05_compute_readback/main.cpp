#include "nvapp_course.hpp"
// 本课重点：复用 storage-buffer 计算流程，比较框架如何提供上下文。
// 入口按课程编号选择路径；实现见 ../../common/nvapp_course.hpp 的生命周期回调。
int main(int argc, char** argv) try {
    return course::nvproMain(argc, argv, 5);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
