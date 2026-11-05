// 关联教材：内存类型、Heap 与分配要求。
// 关键约束：逻辑与实际大小；实际分配可能大于请求。
// 关键约束：选择约束；不能仅按类型编号猜位置。
// 关键约束：接口对齐；不同于一般 allocation alignment。
// vk47_allocation_requirements
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/native_app.hpp"
#include <numeric>

namespace course {
void inspection(const VkContextRef& c) {
    VkBufferResource buffer;
    buffer.init(c, 257, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    VkMemoryRequirements r{};
    vkGetBufferMemoryRequirements(c.device, buffer.buffer, &r);
    require(r.size >= 257 && r.alignment > 0, "Invalid memory requirements");
    std::cout << "requested=257, allocation=" << r.size << ", alignment=" << r.alignment
              << ", memoryTypeBits=" << r.memoryTypeBits << '\n';
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 47);
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
