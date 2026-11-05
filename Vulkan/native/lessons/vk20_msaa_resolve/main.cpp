// 关联教材：帧缓冲、RTV/DSV 与尺寸相关资源。
// 关键约束：颜色与深度；不能只更新颜色附件。
// 关键约束：呈现目标；MSAA 先画独立目标再 resolve。
// 关键约束：重建引用；无在途引用后才释放旧目标。
// vk20_msaa_resolve
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/native_app.hpp"
#include <numeric>

namespace course {
// 附加资源由 SceneVk 的 extension 拥有，在设备仍有效且 GPU 空闲时随场景释放。

int graphicsLab(int argc, char** argv, const LabOptions& options) {
    int mode = ((6));
    Geometry mesh;
    NativeRenderOptions render;
    render.captureStem = "vk_lab_" + std::to_string(20);
    render.redOnly = false;
    render.cull = (VK_CULL_MODE_NONE);
    if (!mesh.vertices.empty())
        render.mesh = &mesh;
    {
        render.samples = VK_SAMPLE_COUNT_4_BIT;
    }
    render.setup = [=](SceneVk& scene) {
        {
            VkPhysicalDeviceProperties p{};
            vkGetPhysicalDeviceProperties(scene.c.physical, &p);
            require((p.limits.framebufferColorSampleCounts & p.limits.framebufferDepthSampleCounts &
                     VK_SAMPLE_COUNT_4_BIT) != 0,
                    "4x MSAA unsupported");
        }
    };

    return nativeMain(argc, argv, mode, render);
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 20);
    nativeShaders = options.directory / "shaders";
    {
        return graphicsLab(argc, argv, options);
    }
}
} // namespace course

int main(int argc, char** argv) try {
    return course::runChapter(argc, argv);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
