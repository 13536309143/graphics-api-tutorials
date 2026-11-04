#include "native_app.hpp"
// 本课重点：使用索引绘制与帧 UBO，比较每帧和每次绘制的数据。
// 入口按课程编号选择路径；实现见 ../../common/native_app.hpp 和 ../../../common/scene_vk.hpp。
int main(int argc, char** argv) try {
    return course::nativeMain(argc, argv, 5);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
