#include "nvapp_course.hpp"
// 本课重点：由 nvvk::Context 管理实例、设备与队列，比较原生初始化过程。
// 入口按课程编号选择路径；实现见 ../../common/nvapp_course.hpp 的生命周期回调。
int main(int argc, char** argv) try {
    return course::nvproMain(argc, argv, 1);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
