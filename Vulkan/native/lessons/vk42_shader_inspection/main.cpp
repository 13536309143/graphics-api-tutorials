// 关联教材：着色器编译、模块与反射。
// 关键约束：产物表示；源码文本不是最终模块。
// 关键约束：组大小证据；host dispatch 与 shader 约定一致。
// 关键约束：缓存输入；同源码 Debug 重编译未必字节相同。
// vk42_shader_inspection
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/native_app.hpp"
#include <numeric>

namespace course {
void inspection(const VkContextRef& c) {
    auto data = spirv(nativeShaders / "lab.comp.spv");
    require(data[0] == 0x07230203, "Wrong SPIR-V magic");
    auto module = shaderModule(c.device, nativeShaders / "lab.comp.spv");
    vkDestroyShaderModule(c.device, module, nullptr);
    std::cout << "Verified SPIR-V module, words=" << data.size() << ", bound=" << data[3] << '\n';
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 42);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            inspection(c);
        }
    }
    std::cout << "Validation errors: " << validationErrors << '\n';
    return validationErrors ? 1 : 0;
}
} // namespace course

int main(int argc, char** argv) try {
    return course::runChapter(argc, argv);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
