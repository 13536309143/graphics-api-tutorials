// Storage/UAV、边界与 257 项实验
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：尾部范围；最后仅一项有效。
// 本章检查：安全条件；容量、组数、guard、CPU 检查一起更新。
// 本章检查：读回证据；首元素正确不足以证明覆盖。
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

// 一个计算实验拥有自己的输出、常量与描述符；动态槽以设备要求的 UBO 对齐组织。
struct ComputeKernel {
    VkContextRef c;
    VkBufferResource output, parameters;
    VkDescriptorSetLayout layout{};
    VkDescriptorPool pool{};
    VkDescriptorSet set{};
    VkPipelineLayout pipelineLayout{};
    VkPipeline pipeline{};
    uint32_t stride{};
    explicit ComputeKernel(VkContextRef context, VkPipelineCache cache = {}) : c(context) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(c.physical, &props);
        stride = uint32_t(alignUp(16, props.limits.minUniformBufferOffsetAlignment));
        output.init(c, 1024 * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        parameters.init(c, stride * 3, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        std::vector<uint8_t> bytes(stride * 3);
        const uint32_t pairs[3][4] = {{2, 3, 0, 0}, {5, 11, 0, 0}, {7, 13, 0, 0}};
        for (uint32_t i = 0; i < 3; ++i)
            std::memcpy(bytes.data() + stride * i, pairs[i], 16);
        parameters.upload(bytes.data(), bytes.size());
        VkDescriptorSetLayoutBinding bindings[2] = {
            {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
            {1,
             VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
             1,
             VK_SHADER_STAGE_COMPUTE_BIT,
             nullptr}};
        VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        li.bindingCount = 2;
        li.pBindings = bindings;
        // 绑定契约：set/binding、描述符类型和 shader 可见阶段必须与 GLSL 一致。
        vkCheck(vkCreateDescriptorSetLayout(c.device, &li, nullptr, &layout));
        VkDescriptorPoolSize sizes[2] = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1}};
        VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pi.maxSets = 1;
        pi.poolSizeCount = 2;
        pi.pPoolSizes = sizes;
        vkCheck(vkCreateDescriptorPool(c.device, &pi, nullptr, &pool));
        VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        ai.descriptorPool = pool;
        ai.descriptorSetCount = 1;
        ai.pSetLayouts = &layout;
        vkCheck(vkAllocateDescriptorSets(c.device, &ai, &set));
        VkDescriptorBufferInfo infos[2] = {{output.buffer, 0, output.size},
                                           {parameters.buffer, 0, 16}};
        VkWriteDescriptorSet writes[2]{};
        for (uint32_t i = 0; i < 2; ++i) {
            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet = set;
            writes[i].dstBinding = i;
            writes[i].descriptorCount = 1;
            writes[i].descriptorType = bindings[i].descriptorType;
            writes[i].pBufferInfo = &infos[i];
        }
        vkUpdateDescriptorSets(c.device, 2, writes, 0, nullptr);
        VkPushConstantRange range{VK_SHADER_STAGE_COMPUTE_BIT, 0, 16};
        VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pl.setLayoutCount = 1;
        pl.pSetLayouts = &layout;
        pl.pushConstantRangeCount = 1;
        pl.pPushConstantRanges = &range;
        vkCheck(vkCreatePipelineLayout(c.device, &pl, nullptr, &pipelineLayout));
        auto shader = shaderModule(c.device, nativeShaders / "lab.comp.spv");
        VkComputePipelineCreateInfo ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        ci.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    nullptr,
                    0,
                    VK_SHADER_STAGE_COMPUTE_BIT,
                    shader,
                    "main",
                    nullptr};
        ci.layout = pipelineLayout;
        auto r = vkCreateComputePipelines(c.device, cache, 1, &ci, nullptr, &pipeline);
        vkDestroyShaderModule(c.device, shader, nullptr);
        vkCheck(r);
    }
    void record(VkCommandBuffer cmd,
                uint32_t mode,
                uint32_t count,
                uint32_t factor,
                uint32_t bias,
                uint32_t slot = 0) {
        uint32_t constants[4] = {mode, count, factor, bias};
        uint32_t offset = slot * stride;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &set, 1, &offset);
        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 16, constants);
        // 工作组数量向上取整；shader 必须检查尾部线程，避免越界访问。
        // Dispatch 的单位是工作组，线程组尺寸由 HLSL numthreads 声明。
        vkCmdDispatch(cmd, (count + 63) / 64, 1, 1);
        bufferDependency(cmd,
                         output.buffer,
                         VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                         VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_HOST_BIT,
                         VK_ACCESS_2_HOST_READ_BIT);
    }
    void execute(uint32_t mode, uint32_t count, uint32_t factor, uint32_t bias, uint32_t slot = 0) {
        immediate(c, [&](VkCommandBuffer cmd) {
            record(cmd, mode, count, factor, bias, slot);
        });
    }
    ~ComputeKernel() {
        vkDestroyPipeline(c.device, pipeline, nullptr);
        vkDestroyPipelineLayout(c.device, pipelineLayout, nullptr);
        vkDestroyDescriptorPool(c.device, pool, nullptr);
        vkDestroyDescriptorSetLayout(c.device, layout, nullptr);
    }
};

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 28);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            ComputeKernel kernel(c);
            uint32_t mode = ((0));
            uint32_t count = (257);
            for (uint32_t slot = 0; slot < ((1u)); ++slot) {
                uint32_t factor = (3), bias = (7);
                kernel.execute(mode, count, factor, bias, slot);
                auto result = readWords(kernel.output, mode == 2 ? (count + 63) / 64 : count);
                if (mode == 2) {
                    uint32_t total = std::accumulate(result.begin(), result.end(), 0u);
                    require(total == count * (count + 1) / 2, "Shared-memory reduction mismatch");
                } else {
                    if (mode == 1) {
                        factor = slot == 0 ? 2 : slot == 1 ? 5 : 7;
                        bias = slot == 0 ? 3 : slot == 1 ? 11 : 13;
                    }
                    for (uint32_t i = 0; i < count; ++i)
                        require(result[i] == i * factor + bias,
                                "Compute/dynamic-binding result mismatch");
                }
            }
            std::cout << "Verified experiment " << 28 << ", items=" << count
                      << ", uniform stride=" << kernel.stride << '\n';
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
