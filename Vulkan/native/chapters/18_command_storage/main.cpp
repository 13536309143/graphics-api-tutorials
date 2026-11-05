// 命令池、Allocator 与安全重置
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：命令使用期；Reset 的安全条件来自 GPU 进度。
// 本章检查：重置粒度；对象与存储的规则分别核对。
// 本章检查：四代验证；每代核对，避免只看最后一代。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/native_app.hpp"
#include <numeric>

namespace course {
// 缓冲屏障按真正的生产/消费访问推导；coherent 只简化缓存管理，不代替执行依赖。
void bufferDependency(VkCommandBuffer cmd,
                      VkBuffer buffer,
                      VkPipelineStageFlags2 before,
                      VkAccessFlags2 write,
                      VkPipelineStageFlags2 after,
                      VkAccessFlags2 read) {
    VkBufferMemoryBarrier2 b{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    b.srcStageMask = before;
    b.srcAccessMask = write;
    b.dstStageMask = after;
    b.dstAccessMask = read;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.buffer = buffer;
    b.offset = 0;
    b.size = VK_WHOLE_SIZE;
    VkDependencyInfo d{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    d.bufferMemoryBarrierCount = 1;
    d.pBufferMemoryBarriers = &b;
    // 可见性依赖：源阶段/访问描述生产者，目标阶段/访问描述接下来的消费者。
    vkCmdPipelineBarrier2(cmd, &d);
}

std::vector<uint32_t> readWords(VkBufferResource& buffer, size_t count) {
    void* p{};
    vkCheck(vkMapMemory(buffer.device, buffer.memory, 0, buffer.size, 0, &p));
    std::vector<uint32_t> result(static_cast<uint32_t*>(p), static_cast<uint32_t*>(p) + count);
    vkUnmapMemory(buffer.device, buffer.memory);
    return result;
}

// 整池重置必须发生在该代命令完成之后；每代写不同数据以发现旧命令残留。
void commandReuse(const VkContextRef& c) {
    VkBufferResource output;
    output.init(c, 256, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pi.queueFamilyIndex = c.family;
    VkCommandPool pool{};
    vkCheck(vkCreateCommandPool(c.device, &pi, nullptr, &pool));
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = pool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    VkCommandBuffer cmd{};
    vkCheck(vkAllocateCommandBuffers(c.device, &ai, &cmd));
    VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence fence{};
    vkCheck(vkCreateFence(c.device, &fi, nullptr, &fence));
    for (uint32_t generation = 0; generation < 4; ++generation) {
        vkCheck(vkResetCommandPool(c.device, pool, 0));
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkCheck(vkBeginCommandBuffer(cmd, &bi));
        vkCmdFillBuffer(cmd, output.buffer, 0, 256, 41 + generation);
        bufferDependency(cmd,
                         output.buffer,
                         VK_PIPELINE_STAGE_2_CLEAR_BIT,
                         VK_ACCESS_2_TRANSFER_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_HOST_BIT,
                         VK_ACCESS_2_HOST_READ_BIT);
        vkCheck(vkEndCommandBuffer(cmd));
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        // 提交只把工作交给 GPU；复用命令、常量或释放资源前仍需确认完成。
        vkCheck(vkQueueSubmit(c.queue, 1, &submit, fence));
        vkCheck(vkWaitForFences(c.device, 1, &fence, VK_TRUE, UINT64_MAX));
        for (auto x : readWords(output, 64))
            require(x == 41 + generation, "Reset command storage contains stale data");
        vkCheck(vkResetFences(c.device, 1, &fence));
    }
    vkDestroyFence(c.device, fence, nullptr);
    vkDestroyCommandPool(c.device, pool, nullptr);
    std::cout << "Verified 4 generations of command-pool reset after completion\n";
}

void inspection(const VkContextRef& c) {
    commandReuse(c);
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 44);
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
