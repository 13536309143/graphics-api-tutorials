#include "nvapp_course.hpp"
// 本课重点：由 nvapp 驱动应用生命周期，把清屏结果放入可交互视口。
// 入口按课程编号选择路径；实现见 ../../common/nvapp_course.hpp 的生命周期回调。
int main(int argc, char** argv) try {
    return course::nvproMain(argc, argv, 2);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
