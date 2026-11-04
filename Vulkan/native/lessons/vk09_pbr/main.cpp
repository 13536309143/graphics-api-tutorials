#include "native_app.hpp"
// 本课重点：用 push constants 改变球阵列的金属度和粗糙度，计算直接光 PBR。
// 入口按课程编号选择路径；实现见 ../../common/native_app.hpp 和 ../../../common/scene_vk.hpp。
int main(int argc, char** argv) try {
    return course::nativeMain(argc, argv, 9);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
