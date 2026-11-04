#include "native_app.hpp"
// 本课重点：使用深度附件与实例索引，一次提交绘制多个立方体。
// 入口按课程编号选择路径；实现见 ../../common/native_app.hpp 和 ../../../common/scene_vk.hpp。
int main(int argc, char** argv) try {
    return course::nativeMain(argc, argv, 7);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
