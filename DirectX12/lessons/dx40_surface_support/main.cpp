// 关联教材：窗口 Surface 与 HWND 连接。
// 关键约束：平台句柄；窗口不是纹理资源。
// 关键约束：呈现支持；graphics 能力不独自证明呈现成立。
// 关键约束：生命周期；先结束 GPU/呈现使用再释放依赖。
// dx40_surface_support
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {
void surfaceInspection(const LabOptions& options) {
    require(glfwInit() != 0, "GLFW init failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto* window = glfwCreateWindow(960, 640, "DXGI inspection", nullptr, nullptr);
    require(window != nullptr, "Window failed");
    {
        DxWindow app(window, 2, options.warp);
        DXGI_SWAP_CHAIN_DESC1 desc{};
        dxCheck(app.swap->GetDesc1(&desc));
        require(desc.BufferCount == 2 && desc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD,
                "Unexpected swapchain configuration");
        app.render(1, nullptr);
        std::cout << "Verified HWND swapchain, buffers=" << desc.BufferCount
                  << ", index=" << app.swap->GetCurrentBackBufferIndex() << '\n';
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}

// 查询、诊断与字节检查有明确的结果；合法 INFO 消息可能被默认过滤，临时空过滤器只扩大接收范围。
void inspection(DxDevice& c, const LabOptions& options) {
    throw std::runtime_error("Unknown inspection id");
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 40);
    {
        surfaceInspection(options);
        return 0;
    }
}
} // namespace course

int main(int argc, char** argv) try {
    return course::runChapter(argc, argv);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
