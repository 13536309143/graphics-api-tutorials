#include "dx12_course.hpp"
// 本课重点：通过 SV_VertexID 生成顶点，连接 root signature 和 PSO。
// 入口按课程编号选择路径；实现见 ../../common/dx12_course.hpp。
int main(int argc, char** argv) try {
    return course::dxMain(argc, argv, 3);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
