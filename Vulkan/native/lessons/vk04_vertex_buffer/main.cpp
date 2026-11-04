#include "native_app.hpp"
// 本课重点：创建并上传顶点缓冲，核对 attribute offset 与 Vertex 布局。
// 入口按课程编号选择路径；实现见 ../../common/native_app.hpp 和 ../../../common/scene_vk.hpp。
int main(int argc, char** argv) try {
    return course::nativeMain(argc, argv, 4);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
