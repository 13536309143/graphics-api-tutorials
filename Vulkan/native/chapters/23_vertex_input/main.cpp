// 顶点语义、Stride 与 Offset
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：顶点尺寸；position/normal/UV 连续契约。
// 本章检查：属性偏移；format 与 stride 一起正确。
// 本章检查：输入编号；与 descriptor/register 是不同系统。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/native_app.hpp"
#include <numeric>

namespace course {
// 附加资源由 SceneVk 的 extension 拥有，在设备仍有效且 GPU 空闲时随场景释放。

int graphicsLab(int argc, char** argv, const LabOptions& options) {
    int mode = ((4));
    Geometry mesh;
    {
        mesh = geometry(4);
        for (auto& vertex : mesh.vertices) {
            vertex.normal = (glm::vec3(1 - vertex.uv.x, vertex.uv.y, 1));
            vertex.uv *= 4.f;
        }
    }
    NativeRenderOptions render;
    render.captureStem = "vk_lab_" + std::to_string(53);
    render.redOnly = false;
    render.cull = (VK_CULL_MODE_NONE);
    if (!mesh.vertices.empty())
        render.mesh = &mesh;
    render.setup = [=](SceneVk& scene) {
        // 这两课直接使用数学裁剪坐标；显式翻转 Y，使正高度 Vulkan viewport 与 DX12 映射一致。
        {
            scene.frameProvider = [](float, float, bool vulkan) {
                Frame result;
                if (vulkan)
                    result.viewProjection[1][1] = -1;
                return result;
            };
        }
    };

    return nativeMain(argc, argv, mode, render);
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 53);
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
