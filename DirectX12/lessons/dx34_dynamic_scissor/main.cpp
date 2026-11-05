// 关联教材：固定功能、颜色掩码、视口与剔除。
// 关键约束：映射范围；半视口改变 NDC 到像素的映射。
// 关键约束：写入限制；半 scissor 不压缩几何。
// 关键约束：不可动态配置；需匹配所创建的管线组合。
// 关键约束：正面绕序；读回图核对外表面；两边默认零值不等价。
// dx34_dynamic_scissor
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {

int graphicsLab(int argc, char** argv, const LabOptions& options) {
    int mode = ((6));
    Geometry mesh;
    DxRenderOptions render;
    render.captureStem = "dx_lab_" + std::to_string(34);
    render.redOnly = false;
    render.cull = (D3D12_CULL_MODE_NONE);
    render.clampSampler = false;
    if (!mesh.vertices.empty())
        render.mesh = &mesh;
    render.setup = [=](DxScene& scene) {
        {
            scene.halfScissor = true;
        }
    };
    // 计算先独立提交完成，再开始图形命令录制，避免在同一 allocator 正录制时把它重置。
    render.beforeFrame = [](DxScene& scene, float time) {
        // The compute submission must finish before DxWindow resets its allocator to record graphics.
    };
    return dxMain(argc, argv, mode, render);
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 34);
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
