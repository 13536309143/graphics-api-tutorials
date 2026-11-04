#include "dx12_course.hpp"
// 本课重点：按 footprint 上传纹理，并通过可见 descriptor heap 的 SRV 采样。
// 入口按课程编号选择路径；实现见 ../../common/dx12_course.hpp。
int main(int argc, char** argv) try {
    return course::dxMain(argc, argv, 6);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
