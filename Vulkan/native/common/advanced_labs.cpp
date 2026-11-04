#define STB_IMAGE_IMPLEMENTATION
#include "advanced.hpp"
#include "advanced_labs.hpp"
#include "native_app.hpp"
#include <numeric>

namespace course {
namespace {
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
void capabilities(const VkContextRef& c) {
    VkPhysicalDeviceProperties p{};
    vkGetPhysicalDeviceProperties(c.physical, &p);
    VkPhysicalDeviceMemoryProperties m{};
    vkGetPhysicalDeviceMemoryProperties(c.physical, &m);
    std::cout << "maxPushConstantsSize=" << p.limits.maxPushConstantsSize
              << ", minUniformBufferOffsetAlignment=" << p.limits.minUniformBufferOffsetAlignment
              << ", maxComputeWorkGroupInvocations=" << p.limits.maxComputeWorkGroupInvocations
              << '\n';
    for (uint32_t i = 0; i < m.memoryTypeCount; ++i)
        std::cout << "memoryType[" << i << "] heap=" << m.memoryTypes[i].heapIndex
                  << " properties=" << m.memoryTypes[i].propertyFlags << '\n';
    require(p.limits.maxPushConstantsSize >= 80, "80-byte Draw exceeds device limit");
    VkFormatProperties depth{};
    vkGetPhysicalDeviceFormatProperties(c.physical, VK_FORMAT_D32_SFLOAT, &depth);
    require((depth.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0,
            "D32 unsupported");
    std::cout << "Verified required limits and depth format\n";
}
// upload → device-local → readback 完整闭环；每次拷贝后的消费者访问不同。
void stagingCopy(const VkContextRef& c) {
    constexpr size_t count = 257;
    std::vector<uint32_t> original(count);
    for (uint32_t i = 0; i < count; ++i)
        original[i] = i * 17 + 9;
    VkBufferResource upload, gpu, readback;
    upload.init(c, count * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    upload.upload(original.data(), count * 4);
    gpu.init(c,
             count * 4,
             VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    readback.init(c, count * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    immediate(c, [&](VkCommandBuffer cmd) {
        VkBufferCopy copy{0, 0, count * 4};
        vkCmdCopyBuffer(cmd, upload.buffer, gpu.buffer, 1, &copy);
        bufferDependency(cmd,
                         gpu.buffer,
                         VK_PIPELINE_STAGE_2_COPY_BIT,
                         VK_ACCESS_2_TRANSFER_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_COPY_BIT,
                         VK_ACCESS_2_TRANSFER_READ_BIT);
        vkCmdCopyBuffer(cmd, gpu.buffer, readback.buffer, 1, &copy);
        bufferDependency(cmd,
                         readback.buffer,
                         VK_PIPELINE_STAGE_2_COPY_BIT,
                         VK_ACCESS_2_TRANSFER_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_HOST_BIT,
                         VK_ACCESS_2_HOST_READ_BIT);
    });
    require(readWords(readback, count) == original, "Device-local staging copy mismatch");
    std::cout << "Verified 257 device-local round-trip words\n";
}
// 两份独立资源交替在途，重用前等待对应 fence；这不是窗口交换链的完整多帧实现。
void frameRing(const VkContextRef& c) {
    // Two independent storage buffers and command buffers remain in flight.
    // This isolates frame-slot lifetime; windowed renderers still use the simple serialized path.
    ComputeKernel kernels[2] = {ComputeKernel(c), ComputeKernel(c)};
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pi.queueFamilyIndex = c.family;
    pi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool pool{};
    vkCheck(vkCreateCommandPool(c.device, &pi, nullptr, &pool));
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = pool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 2;
    VkCommandBuffer commands[2]{};
    vkCheck(vkAllocateCommandBuffers(c.device, &ai, commands));
    VkFence fences[2]{};
    VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (auto& fence : fences)
        vkCheck(vkCreateFence(c.device, &fi, nullptr, &fence));
    int previous[2] = {-1, -1};
    for (uint32_t frameNumber = 0; frameNumber < 8; ++frameNumber) {
        uint32_t slot = frameNumber % 2;
        vkCheck(vkWaitForFences(c.device, 1, &fences[slot], VK_TRUE, UINT64_MAX));
        if (previous[slot] >= 0) {
            auto result = readWords(kernels[slot].output, 257);
            for (uint32_t i = 0; i < 257; ++i)
                require(result[i] == i * 3 + uint32_t(previous[slot]),
                        "In-flight slot overwritten");
        }
        vkCheck(vkResetFences(c.device, 1, &fences[slot]));
        vkCheck(vkResetCommandBuffer(commands[slot], 0));
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkCheck(vkBeginCommandBuffer(commands[slot], &bi));
        kernels[slot].record(commands[slot], 0, 257, 3, frameNumber);
        vkCheck(vkEndCommandBuffer(commands[slot]));
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &commands[slot];
        vkCheck(vkQueueSubmit(c.queue, 1, &submit, fences[slot]));
        previous[slot] = int(frameNumber);
    }
    vkCheck(vkWaitForFences(c.device, 2, fences, VK_TRUE, UINT64_MAX));
    for (uint32_t slot = 0; slot < 2; ++slot) {
        auto result = readWords(kernels[slot].output, 257);
        for (uint32_t i = 0; i < 257; ++i)
            require(result[i] == i * 3 + uint32_t(previous[slot]), "Final slot mismatch");
        vkDestroyFence(c.device, fences[slot], nullptr);
    }
    vkDestroyCommandPool(c.device, pool, nullptr);
    std::cout << "Verified 8 submissions using 2 independent in-flight slots\n";
}
// GPU 先 signal 1，下一提交 wait 1 / signal 2；CPU 等 2 后才能宣告链路完成。
void timelineCounter(const VkContextRef& c) {
    VkSemaphoreTypeCreateInfo type{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
    type.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    VkSemaphoreCreateInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    info.pNext = &type;
    VkSemaphore semaphore{};
    vkCheck(vkCreateSemaphore(c.device, &info, nullptr, &semaphore));
    VkSemaphoreSubmitInfo signal{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal.semaphore = semaphore;
    signal.value = 1;
    signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkSubmitInfo2 first{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    first.signalSemaphoreInfoCount = 1;
    first.pSignalSemaphoreInfos = &signal;
    vkCheck(vkQueueSubmit2(c.queue, 1, &first, VK_NULL_HANDLE));
    VkSemaphoreSubmitInfo wait = signal;
    signal.value = 2;
    VkSubmitInfo2 second{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    second.waitSemaphoreInfoCount = 1;
    second.pWaitSemaphoreInfos = &wait;
    second.signalSemaphoreInfoCount = 1;
    second.pSignalSemaphoreInfos = &signal;
    vkCheck(vkQueueSubmit2(c.queue, 1, &second, VK_NULL_HANDLE));
    uint64_t target = 2;
    VkSemaphoreWaitInfo cpu{VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
    cpu.semaphoreCount = 1;
    cpu.pSemaphores = &semaphore;
    cpu.pValues = &target;
    vkCheck(vkWaitSemaphores(c.device, &cpu, UINT64_MAX));
    uint64_t counter{};
    vkCheck(vkGetSemaphoreCounterValue(c.device, semaphore, &counter));
    require(counter >= 2, "Timeline did not advance");
    vkCheck(vkQueueWaitIdle(c.queue));
    vkDestroySemaphore(c.device, semaphore, nullptr);
    std::cout << "Verified timeline producer 1 -> consumer 2, counter=" << counter << '\n';
}
// timestampValidBits 和 timestampPeriod 决定 tick 的解释，不把单次实验当 API 性能排名。
void timestamps(const VkContextRef& c) {
    uint32_t count{};
    vkGetPhysicalDeviceQueueFamilyProperties(c.physical, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(c.physical, &count, families.data());
    require(families[c.family].timestampValidBits != 0,
            "Selected queue does not support timestamps");
    VkQueryPoolCreateInfo info{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    info.queryCount = 2;
    VkQueryPool pool{};
    vkCheck(vkCreateQueryPool(c.device, &info, nullptr, &pool));
    VkBufferResource buffer;
    buffer.init(
        c, 1024 * 1024, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    immediate(c, [&](VkCommandBuffer cmd) {
        vkCmdResetQueryPool(cmd, pool, 0, 2);
        vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, pool, 0);
        vkCmdFillBuffer(cmd, buffer.buffer, 0, VK_WHOLE_SIZE, 0x12345678);
        vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, pool, 1);
    });
    uint64_t ticks[2]{};
    vkCheck(vkGetQueryPoolResults(c.device,
                                  pool,
                                  0,
                                  2,
                                  sizeof(ticks),
                                  ticks,
                                  8,
                                  VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
    VkPhysicalDeviceProperties p{};
    vkGetPhysicalDeviceProperties(c.physical, &p);
    auto bits = families[c.family].timestampValidBits;
    uint64_t mask = bits == 64 ? UINT64_MAX : ((uint64_t(1) << bits) - 1);
    std::cout << "GPU fill measurement="
              << ((ticks[1] - ticks[0]) & mask) * p.limits.timestampPeriod / 1e6
              << " ms (single operation, not an API benchmark)\n";
    vkDestroyQueryPool(c.device, pool, nullptr);
}
void imageDependency(VkCommandBuffer cmd,
                     VkImage image,
                     uint32_t level,
                     VkImageLayout before,
                     VkImageLayout after,
                     VkPipelineStageFlags2 src,
                     VkAccessFlags2 write,
                     VkPipelineStageFlags2 dst,
                     VkAccessFlags2 read) {
    VkImageMemoryBarrier2 b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    b.srcStageMask = src;
    b.srcAccessMask = write;
    b.dstStageMask = dst;
    b.dstAccessMask = read;
    b.oldLayout = before;
    b.newLayout = after;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image;
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, level, 1, 0, 1};
    VkDependencyInfo d{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    d.imageMemoryBarrierCount = 1;
    d.pImageMemoryBarriers = &b;
    vkCmdPipelineBarrier2(cmd, &d);
}
// 图像、view、sampler 分层持有；逐级 blit 的 mip 状态分别跟踪，生成后再允许采样。
struct GpuTexture {
    VkContextRef c;
    VkImageResource image;
    VkSampler sampler{};
    VkImageView selectedView{};
    VkFormat format{};
    std::vector<Pixels> reference;
    explicit GpuTexture(
        VkContextRef context, Pixels pixels, bool mips, bool srgb = false, bool clamp = false)
        : c(context) {
        reference = mips ? mipReference(pixels, srgb) : std::vector<Pixels>{pixels};
        uint32_t levels = uint32_t(reference.size());
        format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
        if (mips) {
            VkFormatProperties properties{};
            vkGetPhysicalDeviceFormatProperties(c.physical, format, &properties);
            VkFormatFeatureFlags required = VK_FORMAT_FEATURE_BLIT_SRC_BIT |
                                            VK_FORMAT_FEATURE_BLIT_DST_BIT |
                                            VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
            require((properties.optimalTilingFeatures & required) == required,
                    "Format cannot generate linear-blit mip chain");
        }
        image.init(c,
                   {pixels.width, pixels.height},
                   format,
                   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                       VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                   VK_IMAGE_ASPECT_COLOR_BIT,
                   levels);
        VkBufferResource upload;
        upload.init(c, pixels.rgba.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        upload.upload(pixels.rgba.data(), pixels.rgba.size());
        immediate(c, [&](VkCommandBuffer cmd) {
            for (uint32_t i = 0; i < levels; ++i)
                imageDependency(cmd,
                                image.image,
                                i,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_PIPELINE_STAGE_2_NONE,
                                0,
                                VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT,
                                VK_ACCESS_2_TRANSFER_WRITE_BIT);
            VkBufferImageCopy copy{};
            copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copy.imageExtent = {pixels.width, pixels.height, 1};
            vkCmdCopyBufferToImage(
                cmd, upload.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
            for (uint32_t i = 1; i < levels; ++i) {
                imageDependency(cmd,
                                image.image,
                                i - 1,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT,
                                VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                VK_PIPELINE_STAGE_2_BLIT_BIT,
                                VK_ACCESS_2_TRANSFER_READ_BIT);
                VkImageBlit blit{};
                blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1};
                blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1};
                blit.srcOffsets[1] = {int(reference[i - 1].width), int(reference[i - 1].height), 1};
                blit.dstOffsets[1] = {int(reference[i].width), int(reference[i].height), 1};
                vkCmdBlitImage(cmd,
                               image.image,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               image.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &blit,
                               VK_FILTER_LINEAR);
            }
            for (uint32_t i = 0; i < levels; ++i)
                imageDependency(cmd,
                                image.image,
                                i,
                                i + 1 == levels ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                                                : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT,
                                VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
        });
        VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        si.magFilter = si.minFilter = VK_FILTER_LINEAR;
        si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        si.addressModeU = si.addressModeV = si.addressModeW =
            clamp ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
        si.maxLod = float(levels - 1);
        vkCheck(vkCreateSampler(c.device, &si, nullptr, &sampler));
        std::cout << "Uploaded image " << pixels.width << 'x' << pixels.height
                  << ", mipLevels=" << levels << ", sRGB=" << srgb << '\n';
    }
    void attach(SceneVk& scene, bool lastMip = false) {
        if (lastMip) {
            VkImageViewCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            ci.image = image.image;
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.format = format;
            ci.subresourceRange = {
                VK_IMAGE_ASPECT_COLOR_BIT, uint32_t(reference.size() - 1), 1, 0, 1};
            vkCheck(vkCreateImageView(c.device, &ci, nullptr, &selectedView));
        }
        VkDescriptorImageInfo info{
            sampler, lastMip ? selectedView : image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = scene.descriptor;
        write.dstBinding = 1;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &info;
        vkUpdateDescriptorSets(c.device, 1, &write, 0, nullptr);
    }
    // 逐级复制到可见缓冲并核对每个 texel，避免只检查第一个级别或最终画面。
    void verify() {
        VkDeviceSize bytes = 0;
        std::vector<VkDeviceSize> offsets;
        for (const auto& p : reference) {
            offsets.push_back(bytes);
            bytes += p.rgba.size();
        }
        VkBufferResource readback;
        readback.init(c, bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        immediate(c, [&](VkCommandBuffer cmd) {
            for (uint32_t i = 0; i < reference.size(); ++i) {
                imageDependency(cmd,
                                image.image,
                                i,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
                                VK_PIPELINE_STAGE_2_COPY_BIT,
                                VK_ACCESS_2_TRANSFER_READ_BIT);
                VkBufferImageCopy copy{};
                copy.bufferOffset = offsets[i];
                copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1};
                copy.imageExtent = {reference[i].width, reference[i].height, 1};
                vkCmdCopyImageToBuffer(cmd,
                                       image.image,
                                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                       readback.buffer,
                                       1,
                                       &copy);
                imageDependency(cmd,
                                image.image,
                                i,
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_PIPELINE_STAGE_2_COPY_BIT,
                                VK_ACCESS_2_TRANSFER_READ_BIT,
                                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
            }
            bufferDependency(cmd,
                             readback.buffer,
                             VK_PIPELINE_STAGE_2_COPY_BIT,
                             VK_ACCESS_2_TRANSFER_WRITE_BIT,
                             VK_PIPELINE_STAGE_2_HOST_BIT,
                             VK_ACCESS_2_HOST_READ_BIT);
        });
        void* mapped{};
        vkCheck(vkMapMemory(c.device, readback.memory, 0, readback.size, 0, &mapped));
        for (uint32_t level = 0; level < reference.size(); ++level) {
            const auto* p = static_cast<const uint8_t*>(mapped) + offsets[level];
            for (size_t i = 0; i < reference[level].rgba.size(); ++i)
                require(std::abs(int(p[i]) - reference[level].rgba[i]) <= 3,
                        "Mip readback differs from CPU reference");
        }
        vkUnmapMemory(c.device, readback.memory);
        std::cout << "Verified every texel of " << reference.size()
                  << " GPU-generated mip levels\n";
    }
    ~GpuTexture() {
        if (selectedView)
            vkDestroyImageView(c.device, selectedView, nullptr);
        if (sampler)
            vkDestroySampler(c.device, sampler, nullptr);
    }
};
// GPU 写出的 32 字节 scalar 顶点直接作为 vertex input，位置不经 CPU 读回再上传。
struct ParticleKernel {
    VkContextRef c;
    VkBufferResource vertices;
    VkDescriptorSetLayout layout{};
    VkDescriptorPool pool{};
    VkDescriptorSet set{};
    VkPipelineLayout pipelineLayout{};
    VkPipeline pipeline{};
    explicit ParticleKernel(VkContextRef context) : c(context) {
        vertices.init(c,
                      96 * 3 * sizeof(Vertex),
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkDescriptorSetLayoutBinding binding{
            0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
        VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        li.bindingCount = 1;
        li.pBindings = &binding;
        vkCheck(vkCreateDescriptorSetLayout(c.device, &li, nullptr, &layout));
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1};
        VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pi.maxSets = 1;
        pi.poolSizeCount = 1;
        pi.pPoolSizes = &size;
        vkCheck(vkCreateDescriptorPool(c.device, &pi, nullptr, &pool));
        VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        ai.descriptorPool = pool;
        ai.descriptorSetCount = 1;
        ai.pSetLayouts = &layout;
        vkCheck(vkAllocateDescriptorSets(c.device, &ai, &set));
        VkDescriptorBufferInfo bi{vertices.buffer, 0, vertices.size};
        VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        w.dstSet = set;
        w.descriptorCount = 1;
        w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        w.pBufferInfo = &bi;
        vkUpdateDescriptorSets(c.device, 1, &w, 0, nullptr);
        VkPushConstantRange range{VK_SHADER_STAGE_COMPUTE_BIT, 0, 8};
        VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pl.setLayoutCount = 1;
        pl.pSetLayouts = &layout;
        pl.pushConstantRangeCount = 1;
        pl.pPushConstantRanges = &range;
        vkCheck(vkCreatePipelineLayout(c.device, &pl, nullptr, &pipelineLayout));
        auto shader = shaderModule(c.device, nativeShaders / "particles.comp.spv");
        VkComputePipelineCreateInfo ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        ci.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    nullptr,
                    0,
                    VK_SHADER_STAGE_COMPUTE_BIT,
                    shader,
                    "main",
                    nullptr};
        ci.layout = pipelineLayout;
        auto r = vkCreateComputePipelines(c.device, {}, 1, &ci, nullptr, &pipeline);
        vkDestroyShaderModule(c.device, shader, nullptr);
        vkCheck(r);
    }
    void execute(float time) {
        struct Tick {
            float time;
            uint32_t count;
        } tick{time, 96};
        immediate(c, [&](VkCommandBuffer cmd) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
            vkCmdBindDescriptorSets(
                cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &set, 0, nullptr);
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 8, &tick);
            vkCmdDispatch(cmd, 5, 1, 1);
            bufferDependency(cmd,
                             vertices.buffer,
                             VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                             VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                             VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
                             VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT);
        });
    }
    ~ParticleKernel() {
        vkDestroyPipeline(c.device, pipeline, nullptr);
        vkDestroyPipelineLayout(c.device, pipelineLayout, nullptr);
        vkDestroyDescriptorPool(c.device, pool, nullptr);
        vkDestroyDescriptorSetLayout(c.device, layout, nullptr);
    }
};
// 附加资源由 SceneVk 的 extension 拥有，在设备仍有效且 GPU 空闲时随场景释放。
struct RenderState {
    std::unique_ptr<GpuTexture> texture;
    std::unique_ptr<ParticleKernel> particles;
    VkBufferResource indirect;
};
int graphicsLab(int argc, char** argv, const LabOptions& options) {
    int id = options.id, mode = id == 27 ? 3 : id == 29 || id == 53 ? 4 : 6;
    Geometry mesh;
    if (id == 21)
        mesh = staticAsset(options.directory / "assets/models/vase.obj");
    if (id == 22 || id == 36)
        mesh = staticAsset(options.directory / "assets/models/robot.gltf");
    if (id == 29)
        mesh = particleGeometry();
    if (id == 52 || id == 53) {
        mesh = geometry(4);
        for (auto& vertex : mesh.vertices) {
            vertex.normal =
                id == 53 ? glm::vec3(1 - vertex.uv.x, vertex.uv.y, 1) : glm::vec3(0, 0, 1);
            vertex.uv *= 4.f;
        }
        if (id == 52)
            for (uint32_t i = 0; i < mesh.vertices.size(); ++i)
                mesh.indices.push_back(i);
    }
    NativeRenderOptions render;
    render.captureStem = "vk_lab_" + std::to_string(id);
    render.redOnly = id == 43;
    render.cull = id == 50 ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    if (!mesh.vertices.empty())
        render.mesh = &mesh;
    if (id == 20 || id == 36)
        render.samples = VK_SAMPLE_COUNT_4_BIT;
    render.setup = [=](SceneVk& scene) {
        auto state = std::make_shared<RenderState>();
        scene.extension = state;
        if (id == 17 || id == 19 || id == 35 || id == 36 || id == 41 || id == 52) {
            auto pixels = imageFile(options.directory / "assets/textures/albedo.png");
            state->texture = std::make_unique<GpuTexture>(scene.c,
                                                          std::move(pixels),
                                                          id == 19 || id == 36 || id == 41,
                                                          id == 35 || id == 36,
                                                          id == 52);
            state->texture->attach(scene, id == 41);
        }
        if (id == 20 || id == 36) {
            VkPhysicalDeviceProperties p{};
            vkGetPhysicalDeviceProperties(scene.c.physical, &p);
            require((p.limits.framebufferColorSampleCounts & p.limits.framebufferDepthSampleCounts &
                     VK_SAMPLE_COUNT_4_BIT) != 0,
                    "4x MSAA unsupported");
        }
        if (id == 27) {
            VkDrawIndirectCommand arguments{3, 1, 0, 0};
            state->indirect.init(scene.c, sizeof(arguments), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
            state->indirect.upload(&arguments, sizeof(arguments));
            scene.indirectArguments = state->indirect.buffer;
        }
        if (id == 29) {
            state->particles = std::make_unique<ParticleKernel>(scene.c);
            scene.overrideVertices = state->particles->vertices.buffer;
        }
        if (id == 34)
            scene.halfScissor = true;
        if (id == 49)
            scene.halfViewport = true;
        if (id == 23 || id == 36)
            scene.frameProvider = [](float aspect, float time, bool vulkan) {
                auto projection = glm::perspectiveRH_ZO(glm::radians(48.f), aspect, .1f, 100.f);
                if (vulkan)
                    projection[1][1] *= -1;
                glm::vec3 eye(std::sin(time * .3f) * 4, 1.6f, std::cos(time * .3f) * 4);
                return Frame{projection * glm::lookAtRH(eye, glm::vec3(0), glm::vec3(0, 1, 0))};
            };
        // 这两课直接使用数学裁剪坐标；显式翻转 Y，使正高度 Vulkan viewport 与 DX12 映射一致。
        if (id == 29 || id == 53)
            scene.frameProvider = [](float, float, bool vulkan) {
                Frame result;
                if (vulkan)
                    result.viewProjection[1][1] = -1;
                return result;
            };
    };
    render.update = [id](SceneVk& scene, float time) {
        if (id == 29)
            std::static_pointer_cast<RenderState>(scene.extension)->particles->execute(time);
    };
    return nativeMain(argc, argv, mode, render);
}
void surfaceInspection() {
    require(glfwInit() != 0, "GLFW init failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto* window = glfwCreateWindow(320, 240, "Surface inspection", nullptr, nullptr);
    require(window != nullptr, "Window failed");
    {
        NativeDevice owner(window);
        VkSurfaceCapabilitiesKHR caps{};
        vkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(owner.c.physical, owner.surface, &caps));
        uint32_t formats{}, modes{};
        vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
            owner.c.physical, owner.surface, &formats, nullptr));
        vkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(
            owner.c.physical, owner.surface, &modes, nullptr));
        require(formats > 0 && modes > 0, "Surface has no formats/present modes");
        std::cout << "Surface minImages=" << caps.minImageCount
                  << ", maxImages=" << caps.maxImageCount << ", formats=" << formats
                  << ", presentModes=" << modes << '\n';
    }
    glfwDestroyWindow(window);
    glfwTerminate();
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
void inspection(const VkContextRef& c, int id) {
    if (id == 37) {
        uint32_t version{};
        vkCheck(vkEnumerateInstanceVersion(&version));
        uint32_t count{};
        vkCheck(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
        std::vector<VkExtensionProperties> extensions(count);
        vkCheck(vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()));
        bool found = false;
        for (const auto& e : extensions) {
            std::cout << e.extensionName << '\n';
            found |= std::strcmp(e.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0;
        }
        require(found, "Debug utils extension missing");
        std::cout << "Loader version=" << VK_API_VERSION_MAJOR(version) << '.'
                  << VK_API_VERSION_MINOR(version) << '\n';
    } else if (id == 38) {
        uint32_t layers{};
        vkCheck(vkEnumerateInstanceLayerProperties(&layers, nullptr));
        std::vector<VkLayerProperties> list(layers);
        vkCheck(vkEnumerateInstanceLayerProperties(&layers, list.data()));
        for (const auto& l : list)
            std::cout << "layer=" << l.layerName << '\n';
        int delivered = 0;
        VkDebugUtilsMessengerCreateInfoEXT info{
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
        info.pUserData = &delivered;
        info.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT,
                                  VkDebugUtilsMessageTypeFlagsEXT,
                                  const VkDebugUtilsMessengerCallbackDataEXT*,
                                  void* user) -> VkBool32 {
            ++*static_cast<int*>(user);
            return VK_FALSE;
        };
        auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(c.instance, "vkCreateDebugUtilsMessengerEXT"));
        auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(c.instance, "vkDestroyDebugUtilsMessengerEXT"));
        auto message = reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>(
            vkGetInstanceProcAddr(c.instance, "vkSubmitDebugUtilsMessageEXT"));
        require(create && destroy && message, "Debug utils functions missing");
        VkDebugUtilsMessengerEXT messenger{};
        vkCheck(create(c.instance, &info, nullptr, &messenger));
        VkDebugUtilsMessengerCallbackDataEXT data{
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT};
        data.pMessage = "Course informational diagnostic";
        message(c.instance,
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
                &data);
        destroy(c.instance, messenger, nullptr);
        require(delivered > 0, "Diagnostic callback was not invoked");
        std::cout << "Verified application informational message delivery\n";
    } else if (id == 39) {
        uint32_t count{};
        vkGetPhysicalDeviceQueueFamilyProperties(c.physical, &count, nullptr);
        std::vector<VkQueueFamilyProperties> queues(count);
        vkGetPhysicalDeviceQueueFamilyProperties(c.physical, &count, queues.data());
        for (uint32_t i = 0; i < count; ++i)
            std::cout << "family=" << i << ", queues=" << queues[i].queueCount
                      << ", flags=" << queues[i].queueFlags
                      << ", timestampBits=" << queues[i].timestampValidBits << '\n';
        require((queues[c.family].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0,
                "Selected family lacks graphics");
    } else if (id == 42) {
        auto data = spirv(nativeShaders / "lab.comp.spv");
        require(data[0] == 0x07230203, "Wrong SPIR-V magic");
        auto module = shaderModule(c.device, nativeShaders / "lab.comp.spv");
        vkDestroyShaderModule(c.device, module, nullptr);
        std::cout << "Verified SPIR-V module, words=" << data.size() << ", bound=" << data[3]
                  << '\n';
    } else if (id == 44)
        commandReuse(c);
    else if (id == 45) {
        ComputeKernel kernel(c);
        kernel.execute(0, 128, 2, 3);
        auto first = readWords(kernel.output, 128);
        VkBufferResource second;
        second.init(c, 4096, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        VkDescriptorBufferInfo info{second.buffer, 0, second.size};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = kernel.set;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &info;
        vkUpdateDescriptorSets(c.device, 1, &write, 0, nullptr);
        immediate(c, [&](VkCommandBuffer cmd) {
            kernel.record(cmd, 0, 128, 5, 11);
            bufferDependency(cmd,
                             second.buffer,
                             VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                             VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                             VK_PIPELINE_STAGE_2_HOST_BIT,
                             VK_ACCESS_2_HOST_READ_BIT);
        });
        auto changed = readWords(second, 128);
        require(readWords(kernel.output, 128) == first, "Descriptor update overwrote prior buffer");
        for (uint32_t i = 0; i < 128; ++i)
            require(changed[i] == i * 5 + 11, "Updated descriptor points at wrong storage");
        std::cout << "Verified descriptor replacement after prior submission completion\n";
    } else if (id == 46) {
        for (auto format :
             {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM}) {
            VkFormatProperties p{};
            vkGetPhysicalDeviceFormatProperties(c.physical, format, &p);
            std::cout << "depth format=" << format
                      << ", optimal features=" << p.optimalTilingFeatures << '\n';
        }
    } else if (id == 47) {
        VkBufferResource buffer;
        buffer.init(c, 257, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        VkMemoryRequirements r{};
        vkGetBufferMemoryRequirements(c.device, buffer.buffer, &r);
        require(r.size >= 257 && r.alignment > 0, "Invalid memory requirements");
        std::cout << "requested=257, allocation=" << r.size << ", alignment=" << r.alignment
                  << ", memoryTypeBits=" << r.memoryTypeBits << '\n';
    } else if (id == 48) {
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VkFence fence{};
        vkCheck(vkCreateFence(c.device, &fi, nullptr, &fence));
        require(vkGetFenceStatus(c.device, fence) == VK_SUCCESS, "Initial fence not signaled");
        vkCheck(vkResetFences(c.device, 1, &fence));
        require(vkGetFenceStatus(c.device, fence) == VK_NOT_READY,
                "Reset fence signaled unexpectedly");
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        vkCheck(vkQueueSubmit(c.queue, 1, &submit, fence));
        vkCheck(vkWaitForFences(c.device, 1, &fence, VK_TRUE, UINT64_MAX));
        vkDestroyFence(c.device, fence, nullptr);
        std::cout << "Verified signaled -> reset -> submitted -> complete fence lifecycle\n";
    } else if (id == 51) {
        static_assert(sizeof(Frame) == 64 && sizeof(Draw) == 80 && offsetof(Draw, settings) == 64);
        ComputeKernel kernel(c);
        kernel.execute(1, 128, 0, 0, 2);
        auto r = readWords(kernel.output, 128);
        for (uint32_t i = 0; i < 128; ++i)
            require(r[i] == i * 7 + 13, "UBO layout mismatch");
        std::cout
            << "Verified Frame=64, Draw=80, settings offset=64 and GPU uniform vector decoding\n";
    } else if (id == 54) {
        capabilities(c);
        stagingCopy(c);
        GpuTexture mips(c, Pixels{64, 64, checker()}, true);
        mips.verify();
        frameRing(c);
        std::cout << "Completed resource/texture/frame-slot acceptance suite\n";
    } else
        throw std::runtime_error("Unknown inspection id");
}

} // namespace
int nativeLabMain(int argc, char** argv, int id) {
    auto options = labOptions(argc, argv, id);
    nativeShaders = options.directory / "shaders";
    if (id == 17 || id == 19 || id == 20 || id == 21 || id == 22 || id == 23 || id == 27 ||
        id == 29 || id == 34 || id == 35 || id == 36 || id == 41 || id == 43 || id == 49 ||
        id == 50 || id == 52 || id == 53)
        return graphicsLab(argc, argv, options);
    validationErrors = 0;
    if (id == 40) {
        surfaceInspection();
        return validationErrors ? 1 : 0;
    }
    {
        NativeDevice owner(nullptr, id == 26);
        const auto& c = owner.c;
        if (id >= 37)
            inspection(c, id);
        else if (id == 11)
            capabilities(c);
        else if (id == 12) {
            VkBufferResource b;
            b.init(c, 256, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
            immediate(c, [&](VkCommandBuffer cmd) {
                vkCmdFillBuffer(cmd, b.buffer, 0, 256, 0x12345678);
                bufferDependency(cmd,
                                 b.buffer,
                                 VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT,
                                 VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                 VK_PIPELINE_STAGE_2_HOST_BIT,
                                 VK_ACCESS_2_HOST_READ_BIT);
            });
            for (auto x : readWords(b, 64))
                require(x == 0x12345678, "Queue fill mismatch");
            std::cout << "Verified queue submission, fence and 64 filled words\n";
        } else if (id == 13)
            stagingCopy(c);
        else if (id == 18) {
            GpuTexture texture(c, Pixels{64, 64, checker()}, true);
            texture.verify();
        } else if (id == 24)
            frameRing(c);
        else if (id == 25)
            timestamps(c);
        else if (id == 26)
            timelineCounter(c);
        else if (id == 31) {
            for (int i = 0; i < 4; ++i)
                stagingCopy(c);
            std::cout << "Retired 4 upload/device/readback generations after completion\n";
        } else if (id == 33) {
            auto begin = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
                vkGetDeviceProcAddr(c.device, "vkCmdBeginDebugUtilsLabelEXT"));
            auto end = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
                vkGetDeviceProcAddr(c.device, "vkCmdEndDebugUtilsLabelEXT"));
            require(begin && end, "Debug label entry points missing");
            immediate(c, [&](VkCommandBuffer cmd) {
                VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
                label.pLabelName = "Course/Upload pass";
                label.color[1] = 1;
                begin(cmd, &label);
                end(cmd);
            });
            std::cout << "Submitted named GPU debug region\n";
        } else if (id == 32) {
            VkPipelineCacheCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
            VkPipelineCache first{}, second{};
            vkCheck(vkCreatePipelineCache(c.device, &info, nullptr, &first));
            {
                ComputeKernel k(c, first);
                k.execute(0, 257, 3, 7);
                auto r = readWords(k.output, 257);
                require(r[256] == 775, "Cached compute output mismatch");
            }
            size_t bytes{};
            vkCheck(vkGetPipelineCacheData(c.device, first, &bytes, nullptr));
            std::vector<uint8_t> data(bytes);
            vkCheck(vkGetPipelineCacheData(c.device, first, &bytes, data.data()));
            info.initialDataSize = bytes;
            info.pInitialData = data.data();
            vkCheck(vkCreatePipelineCache(c.device, &info, nullptr, &second));
            {
                ComputeKernel k(c, second);
                k.execute(0, 257, 3, 7);
                require(readWords(k.output, 257)[256] == 775, "Reloaded pipeline cache mismatch");
            }
            vkDestroyPipelineCache(c.device, second, nullptr);
            vkDestroyPipelineCache(c.device, first, nullptr);
            std::cout << "Verified pipeline-cache serialization and reload, bytes=" << bytes
                      << '\n';
        } else {
            ComputeKernel kernel(c);
            uint32_t mode = id == 30 ? 2 : id == 14 || id == 15 ? 1 : 0;
            uint32_t count = id == 28 || id == 30 ? 257 : 128;
            for (uint32_t slot = 0; slot < (id == 15 ? 3u : 1u); ++slot) {
                uint32_t factor = id == 16 ? 9 : 3, bias = id == 16 ? 17 : 7;
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
            std::cout << "Verified experiment " << id << ", items=" << count
                      << ", uniform stride=" << kernel.stride << '\n';
        }
    }
    std::cout << "Validation errors: " << validationErrors << '\n';
    return validationErrors ? 1 : 0;
}
} // namespace course
