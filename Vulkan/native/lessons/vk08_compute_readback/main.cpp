#include "native_app.hpp"
// 本课重点：dispatch 计算着色器，经 host 可见性屏障和 fence 后核对 256 个结果。
// 入口按课程编号选择路径；实现见 ../../common/native_app.hpp 和 ../../../common/scene_vk.hpp。
int main(int argc, char** argv) try {
    return course::nativeMain(argc, argv, 8);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
