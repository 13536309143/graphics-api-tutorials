// 着色器编译、模块与反射
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：产物表示；源码文本不是最终模块。
// 本章检查：组大小证据；host dispatch 与 shader 约定一致。
// 本章检查：缓存输入；同源码 Debug 重编译未必字节相同。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {
ComPtr<ID3DBlob>
labShader(const LabOptions& options, const char* entry, const char* profile = "cs_5_1") {
    ComPtr<ID3DBlob> result, error;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifndef NDEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    const char* stage = std::strcmp(entry, "ParticleMain") == 0 ? "particles.hlsl"
                        : std::strcmp(entry, "MipMain") == 0    ? "mip.hlsl"
                                                                : "compute_lab.hlsl";
    auto path = options.directory / "shaders" / stage;
    auto hr = D3DCompileFromFile(path.c_str(),
                                 nullptr,
                                 D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                 entry,
                                 profile,
                                 flags,
                                 0,
                                 &result,
                                 &error);
    if (error)
        std::cerr << static_cast<const char*>(error->GetBufferPointer());
    dxCheck(hr, "Lab shader compilation");
    return result;
}

// 查询、诊断与字节检查有明确的结果；合法 INFO 消息可能被默认过滤，临时空过滤器只扩大接收范围。
void inspection(DxDevice& c, const LabOptions& options) {
    auto shader = labShader(options, "ComputeMain");
    ComPtr<ID3D11ShaderReflection> reflection;
    dxCheck(
        D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&reflection)));
    UINT x{}, y{}, z{};
    reflection->GetThreadGroupSize(&x, &y, &z);
    require(x == 64 && y == 1 && z == 1, "Unexpected shader thread-group reflection");
    D3D11_SHADER_DESC desc{};
    dxCheck(reflection->GetDesc(&desc));
    std::cout << "Verified shader reflection: threads=" << x << ", bindings=" << desc.BoundResources
              << '\n';
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 42);
    DxDevice c(options.warp);
    {
        inspection(c, options);
    }
    c.checkMessages();
    return 0;
}
} // namespace course

int main(int argc, char** argv) try {
    return course::runChapter(argc, argv);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
