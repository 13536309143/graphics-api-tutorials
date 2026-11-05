// 帧缓冲、RTV/DSV 与尺寸相关资源
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：颜色与深度；不能只更新颜色附件。
// 本章检查：呈现目标；MSAA 先画独立目标再 resolve。
// 本章检查：重建引用；无在途引用后才释放旧目标。
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
    DxRenderOptions render;
    render.captureStem = "dx_lab_" + std::to_string(20);
    render.redOnly = false;
    render.cull = (D3D12_CULL_MODE_NONE);
    render.clampSampler = false;
    if (!mesh.vertices.empty())
        render.mesh = &mesh;
    {
        render.samples = 4;
    }
    render.setup = [=](DxScene& scene) {
        {
            for (auto format : {DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT}) {
                D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS q{};
                q.Format = format;
                q.SampleCount = 4;
                dxCheck(scene.c.device->CheckFeatureSupport(
                    D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &q, sizeof(q)));
                require(q.NumQualityLevels > 0, "4x MSAA format unsupported");
            }
        }
    };
    // 计算先独立提交完成，再开始图形命令录制，避免在同一 allocator 正录制时把它重置。
    render.beforeFrame = [](DxScene& scene, float time) {
        // The compute submission must finish before DxWindow resets its allocator to record graphics.
    };
    return dxMain(argc, argv, mode, render);
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 20);
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
