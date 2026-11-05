// 关联教材：计算管线、线程组与全局索引。
// 关键约束：工作范围；256 项公式逐项验证。
// 关键约束：可写接口；不走光栅化阶段。
// 关键约束：读回完成；Map 不触发自动 GPU 等待。
// vk08_compute_readback
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
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
        {
            computeSquares(d.c, nativeShaders);
        }
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
