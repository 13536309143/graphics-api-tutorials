#include "native_app.hpp"
// 本课重点：上传纹理并绑定组合采样器，跟踪 transfer 到 shader-read 的依赖。
// 入口按课程编号选择路径；实现见 ../../common/native_app.hpp 和 ../../../common/scene_vk.hpp。
int main(int argc, char** argv) try {
    return course::nativeMain(argc, argv, 6);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
