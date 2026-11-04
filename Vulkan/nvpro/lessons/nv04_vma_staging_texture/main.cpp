#include "nvapp_course.hpp"
// 本课重点：使用 VMA 和 StagingUploader 上传 device-local 顶点，并显示纹理场景。
// 入口按课程编号选择路径；实现见 ../../common/nvapp_course.hpp 的生命周期回调。
int main(int argc, char** argv) try {
    return course::nvproMain(argc, argv, 4);
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
