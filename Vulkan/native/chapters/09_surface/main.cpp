// 窗口 Surface 与 HWND 连接
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：平台句柄；窗口不是纹理资源。
// 本章检查：呈现支持；graphics 能力不独自证明呈现成立。
// 本章检查：生命周期；先结束 GPU/呈现使用再释放依赖。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/native_app.hpp"
#include <numeric>

namespace course {
void surfaceInspection() {
    require(glfwInit() != 0, "GLFW init failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto* window = glfwCreateWindow(320, 240, "Surface inspection", nullptr, nullptr);
    require(window != nullptr, "Window failed");
    {
        NativeDevice owner(window);
        VkSurfaceCapabilitiesKHR caps{};
        vkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(owner.c.physical, owner.surface, &caps));
        uint32_t formats{}, modes{};
        vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
            owner.c.physical, owner.surface, &formats, nullptr));
        vkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(
            owner.c.physical, owner.surface, &modes, nullptr));
        require(formats > 0 && modes > 0, "Surface has no formats/present modes");
        std::cout << "Surface minImages=" << caps.minImageCount
                  << ", maxImages=" << caps.maxImageCount << ", formats=" << formats
                  << ", presentModes=" << modes << '\n';
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}

void inspection(const VkContextRef& c) {
    throw std::runtime_error("Unknown inspection id");
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 40);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        surfaceInspection();
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
