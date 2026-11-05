// 逻辑设备、特性链与基础完成条件
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：查询与启用；应用需求不能超出支持集合。
// 本章检查：基础特性；新效果仍需额外能力判断。
// 本章检查：计数同步特性；Vulkan timeline 不替代 WSI binary 约定。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#include "support/native_app.hpp"
namespace course {
int runChapter(int argc, char** argv) {
    NativeRenderOptions options;

    bool smoke = false;
    for (int i = 1; i < argc; ++i)
        smoke |= std::strcmp(argv[i], "--smoke") == 0;
    validationErrors = 0;
    nativeShaders =
        std::filesystem::absolute(argv[0]).parent_path() / COURSE_RUNTIME_NAME / "shaders";
    // 设备探测和计算课不创建窗口，也不需要 surface 或交换链。
    {
        NativeDevice d(nullptr);
        return validationErrors ? 1 : 0;
    }
}
} // namespace course

int main(int argc, char** argv) try {
    return course::runChapter(argc, argv);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
