// 间接参数、命令签名与可扩展提交
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：间接字节；16-byte 参数布局。
// 本章检查：解释接口；stride/type 与资源内容一致。
// 本章检查：生产者范围；GPU 剔除/命令生成尚未接入。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/native_app.hpp"
#include <numeric>

namespace course {
// 附加资源由 SceneVk 的 extension 拥有，在设备仍有效且 GPU 空闲时随场景释放。
struct RenderState {

    VkBufferResource indirect;
};

int graphicsLab(int argc, char** argv, const LabOptions& options) {
    int mode = (3);
    Geometry mesh;
    NativeRenderOptions render;
    render.captureStem = "vk_lab_" + std::to_string(27);
    render.redOnly = false;
    render.cull = (VK_CULL_MODE_NONE);
    if (!mesh.vertices.empty())
        render.mesh = &mesh;
    render.setup = [=](SceneVk& scene) {
        auto state = std::make_shared<RenderState>();
        scene.extension = state;
        {
            VkDrawIndirectCommand arguments{3, 1, 0, 0};
            state->indirect.init(scene.c, sizeof(arguments), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
            state->indirect.upload(&arguments, sizeof(arguments));
            scene.indirectArguments = state->indirect.buffer;
        }
    };

    return nativeMain(argc, argv, mode, render);
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 27);
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
