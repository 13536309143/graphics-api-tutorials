// 关联教材：固定功能、颜色掩码、视口与剔除。
// 关键约束：映射范围；半视口改变 NDC 到像素的映射。
// 关键约束：写入限制；半 scissor 不压缩几何。
// 关键约束：不可动态配置；需匹配所创建的管线组合。
// 关键约束：正面绕序；读回图核对外表面；两边默认零值不等价。
// vk34_dynamic_scissor
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
    render.captureStem = "vk_lab_" + std::to_string(34);
    render.redOnly = false;
    render.cull = (VK_CULL_MODE_NONE);
    if (!mesh.vertices.empty())
        render.mesh = &mesh;
    render.setup = [=](SceneVk& scene) {
        {
            scene.halfScissor = true;
        }
    };

    return nativeMain(argc, argv, mode, render);
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 34);
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
