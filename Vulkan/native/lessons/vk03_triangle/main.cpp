#include "native_app.hpp"
// 本课重点：不用顶点缓冲，通过 gl_VertexIndex 生成三角形。
// 入口按课程编号选择路径；实现见 ../../common/native_app.hpp 和 ../../../common/scene_vk.hpp。
int main(int argc, char** argv) try {
    return course::nativeMain(argc, argv, 3);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
