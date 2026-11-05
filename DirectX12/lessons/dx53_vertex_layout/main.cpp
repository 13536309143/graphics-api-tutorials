// 关联教材：顶点语义、Stride 与 Offset。
// 关键约束：顶点尺寸；position/normal/UV 连续契约。
// 关键约束：属性偏移；format 与 stride 一起正确。
// 关键约束：输入编号；与 descriptor/register 是不同系统。
// dx53_vertex_layout
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {

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
    DxRenderOptions render;
    render.captureStem = "dx_lab_" + std::to_string(53);
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
    auto options = labOptions(argc, argv, 53);
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
