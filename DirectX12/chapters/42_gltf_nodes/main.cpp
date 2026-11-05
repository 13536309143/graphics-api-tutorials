// glTF 节点层级、法线与静态姿态
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：节点组合；变换只应用一次。
// 本章检查：法线；非均匀缩放下保持垂直关系。
// 本章检查：实现范围；材质/动画扩展另行验收。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {

int graphicsLab(int argc, char** argv, const LabOptions& options) {
    int mode = ((6));
    Geometry mesh;
    {
        mesh = staticAsset(options.directory / "assets/models/robot.gltf");
    }
    DxRenderOptions render;
    render.captureStem = "dx_lab_" + std::to_string(22);
    render.redOnly = false;
    render.cull = (D3D12_CULL_MODE_NONE);
    render.clampSampler = false;
    if (!mesh.vertices.empty())
        render.mesh = &mesh;

    // 计算先独立提交完成，再开始图形命令录制，避免在同一 allocator 正录制时把它重置。
    render.beforeFrame = [](DxScene& scene, float time) {
        // The compute submission must finish before DxWindow resets its allocator to record graphics.
    };
    return dxMain(argc, argv, mode, render);
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 22);
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
