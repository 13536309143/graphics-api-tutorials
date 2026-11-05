#pragma once
#include <vulkan/vulkan.h>
#include "scene.hpp"
#include <functional>
#include <optional>
#include <memory>
namespace course {
inline void vkCheck(VkResult r, const char* message = "Vulkan call") {
    if (r != VK_SUCCESS)
        throw std::runtime_error(std::string(message) + ": " + std::to_string(r));
}

// 只借用句柄，不负责销毁；原生路线和 nvpro 路线可复用同一个底层渲染器。
struct VkContextRef {
    VkInstance instance{};
    VkPhysicalDevice physical{};
    VkDevice device{};
    VkQueue queue{};
    uint32_t family{};
};

// 同时满足资源的 memoryTypeBits 与期望属性，才能把分配内存绑定到资源。
inline uint32_t memoryType(const VkContextRef& c, uint32_t mask, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(c.physical, &props);
    for (uint32_t i = 0; i < props.memoryTypeCount; ++i)
        if ((mask & (1u << i)) && (props.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    throw std::runtime_error("No compatible memory type");
}
struct VkBufferResource {
    VkDevice device{};
    VkBuffer buffer{};
    VkDeviceMemory memory{};
    VkDeviceSize size{};
    VkBufferResource() = default;
    VkBufferResource(const VkBufferResource&) = delete;
    VkBufferResource& operator=(const VkBufferResource&) = delete;
    void init(const VkContextRef& c,
              VkDeviceSize bytes,
              VkBufferUsageFlags usage,
              VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        device = c.device;
        size = bytes;
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size = bytes;
        info.usage = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCheck(vkCreateBuffer(device, &info, nullptr, &buffer));
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(device, buffer, &req);
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc.allocationSize = req.size;
        alloc.memoryTypeIndex = memoryType(c, req.memoryTypeBits, flags);
        vkCheck(vkAllocateMemory(device, &alloc, nullptr, &memory));
        vkCheck(vkBindBufferMemory(device, buffer, memory, 0));
    }

    // 此处缓冲使用 HOST_VISIBLE | HOST_COHERENT；非 coherent 内存还需显式 flush。
    void upload(const void* data, size_t bytes) {
        void* p;
        vkCheck(vkMapMemory(device, memory, 0, size, 0, &p));
        std::memcpy(p, data, bytes);
        vkUnmapMemory(device, memory);
    }
    ~VkBufferResource() {
        if (buffer)
            vkDestroyBuffer(device, buffer, nullptr);
        if (memory)
            vkFreeMemory(device, memory, nullptr);
    }
};
struct VkImageResource {
    VkDevice device{};
    VkImage image{};
    VkImageView view{};
    VkDeviceMemory memory{};
    VkImageResource() = default;
    VkImageResource(const VkImageResource&) = delete;
    VkImageResource& operator=(const VkImageResource&) = delete;
    void init(const VkContextRef& c,
              VkExtent2D size,
              VkFormat format,
              VkImageUsageFlags usage,
              VkImageAspectFlags aspect,
              uint32_t levels = 1,
              VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT) {
        device = c.device;
        VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = format;
        info.extent = {size.width, size.height, 1};
        info.mipLevels = levels;
        info.arrayLayers = 1;
        info.samples = samples;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCheck(vkCreateImage(device, &info, nullptr, &image));
        VkMemoryRequirements req;
        vkGetImageMemoryRequirements(device, image, &req);
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc.allocationSize = req.size;
        alloc.memoryTypeIndex =
            memoryType(c, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkCheck(vkAllocateMemory(device, &alloc, nullptr, &memory));
        vkCheck(vkBindImageMemory(device, image, memory, 0));
        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vi.image = image;
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format = format;
        vi.subresourceRange = {aspect, 0, levels, 0, 1};
        vkCheck(vkCreateImageView(device, &vi, nullptr, &view));
    }
    void reset() {
        if (view)
            vkDestroyImageView(device, view, nullptr);
        if (image)
            vkDestroyImage(device, image, nullptr);
        if (memory)
            vkFreeMemory(device, memory, nullptr);
        view = {};
        image = {};
        memory = {};
    }
    ~VkImageResource() {
        reset();
    }
};

// 图像屏障同时声明布局变化和读写依赖；只有更改 layout 并不足以保证可见性。
inline void barrier(VkCommandBuffer cmd,
                    VkImage image,
                    VkImageLayout before,
                    VkImageLayout after,
                    VkPipelineStageFlags2 srcStage,
                    VkAccessFlags2 srcAccess,
                    VkPipelineStageFlags2 dstStage,
                    VkAccessFlags2 dstAccess,
                    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT) {
    VkImageMemoryBarrier2 b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    b.srcStageMask = srcStage;
    b.srcAccessMask = srcAccess;
    b.dstStageMask = dstStage;
    b.dstAccessMask = dstAccess;
    b.oldLayout = before;
    b.newLayout = after;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image;
    b.subresourceRange = {aspect, 0, 1, 0, 1};
    VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &b;
    // 可见性依赖：源阶段/访问描述生产者，目标阶段/访问描述接下来的消费者。
    vkCmdPipelineBarrier2(cmd, &dependency);
}

// 教学用一次性提交：CPU 等 fence 后再释放命令池及临时上传资源。
inline void immediate(const VkContextRef& c, const std::function<void(VkCommandBuffer)>& record) {
    VkCommandPool pool{};
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pi.queueFamilyIndex = c.family;
    vkCheck(vkCreateCommandPool(c.device, &pi, nullptr, &pool));
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = pool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkCheck(vkAllocateCommandBuffers(c.device, &ai, &cmd));
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkCheck(vkBeginCommandBuffer(cmd, &begin));
    record(cmd);
    vkCheck(vkEndCommandBuffer(cmd));
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence fence;
    vkCheck(vkCreateFence(c.device, &fi, nullptr, &fence));
    // 提交只把工作交给 GPU；复用命令、常量或释放资源前仍需确认完成。
    vkCheck(vkQueueSubmit(c.queue, 1, &submit, fence));
    vkCheck(vkWaitForFences(c.device, 1, &fence, VK_TRUE, UINT64_MAX));
    vkDestroyFence(c.device, fence, nullptr);
    vkDestroyCommandPool(c.device, pool, nullptr);
}
inline VkShaderModule shaderModule(VkDevice device, const std::filesystem::path& path) {
    auto code = spirv(path);
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = code.size() * 4;
    info.pCode = code.data();
    VkShaderModule s;
    vkCheck(vkCreateShaderModule(device, &info, nullptr, &s));
    return s;
}
class SceneVk {
public:
    VkContextRef c;
    int mode;
    VkFormat colorFormat;
    Geometry mesh;
    VkBufferResource vertices, indices, uniform;
    VkImageResource texture;
    VkSampler sampler{};
    VkDescriptorSetLayout setLayout{};
    VkDescriptorPool descriptorPool{};
    VkDescriptorSet descriptor{};
    VkPipelineLayout pipelineLayout{};
    VkPipeline pipeline{};
    VkBuffer overrideVertices{};
    VkBuffer indirectArguments{};
    bool halfScissor = false;
    bool halfViewport = false;
    std::optional<Frame> overrideFrame;
    std::function<Frame(float, float, bool)> frameProvider;
    std::vector<Draw> overrideDraws;
    std::shared_ptr<void> extension;
    SceneVk(const VkContextRef& context,
            int lesson,
            VkFormat format,
            const std::filesystem::path& shaders,
            const Geometry* replacement = nullptr,
            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
            VkPipelineCache cache = VK_NULL_HANDLE,
            VkCullModeFlags cull = VK_CULL_MODE_NONE,
            bool redOnly = false)
        : c(context), mode(lesson), colorFormat(format),
          mesh(replacement ? *replacement : geometry(lesson)) {
        if (mode >= 4) {
            vertices.init(
                c, mesh.vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            vertices.upload(mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
            if (!mesh.indices.empty()) {
                indices.init(c, mesh.indices.size() * 4, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
                indices.upload(mesh.indices.data(), mesh.indices.size() * 4);
            }
        }
        uniform.init(c, sizeof(Frame), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        Frame initial;
        uniform.upload(&initial, sizeof(initial));
        auto data = checker();
        VkBufferResource upload;
        upload.init(c, data.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        upload.upload(data.data(), data.size());
        texture.init(c,
                     {64, 64},
                     VK_FORMAT_R8G8B8A8_SRGB,
                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT);

        // 纹理先切到传输目标布局，拷贝完成后再切到 shader 只读布局供片元阶段采样。
        immediate(c, [&](VkCommandBuffer cmd) {
            barrier(cmd,
                    texture.image,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_PIPELINE_STAGE_2_NONE,
                    0,
                    VK_PIPELINE_STAGE_2_COPY_BIT,
                    VK_ACCESS_2_TRANSFER_WRITE_BIT);
            VkBufferImageCopy copy{};
            copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copy.imageExtent = {64, 64, 1};
            vkCmdCopyBufferToImage(
                cmd, upload.buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
            barrier(cmd,
                    texture.image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_PIPELINE_STAGE_2_COPY_BIT,
                    VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                    VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
        });
        VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        si.magFilter = si.minFilter = VK_FILTER_LINEAR;
        si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        si.maxLod = 0;
        vkCheck(vkCreateSampler(c.device, &si, nullptr, &sampler));

        // binding 0 对应每帧 UBO，binding 1 对应纹理和采样器；编号与 GLSL 保持一致。
        VkDescriptorSetLayoutBinding bindings[2] = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
            {1,
             VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
             1,
             VK_SHADER_STAGE_FRAGMENT_BIT,
             nullptr}};
        VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        li.bindingCount = 2;
        li.pBindings = bindings;
        // 绑定契约：set/binding、描述符类型和 shader 可见阶段必须与 GLSL 一致。
        vkCheck(vkCreateDescriptorSetLayout(c.device, &li, nullptr, &setLayout));
        VkDescriptorPoolSize sizes[2] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
                                         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets = 1;
        pool.poolSizeCount = 2;
        pool.pPoolSizes = sizes;
        vkCheck(vkCreateDescriptorPool(c.device, &pool, nullptr, &descriptorPool));
        VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        da.descriptorPool = descriptorPool;
        da.descriptorSetCount = 1;
        da.pSetLayouts = &setLayout;
        vkCheck(vkAllocateDescriptorSets(c.device, &da, &descriptor));
        VkDescriptorBufferInfo ub{uniform.buffer, 0, sizeof(Frame)};
        VkDescriptorImageInfo image{
            sampler, texture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet writes[2]{};
        for (auto& w : writes) {
            w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w.dstSet = descriptor;
            w.descriptorCount = 1;
        }
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].pBufferInfo = &ub;
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].pImageInfo = &image;
        vkUpdateDescriptorSets(c.device, 2, writes, 0, nullptr);

        // push constants 存放每次绘制的小块数据，更新时不需要再分配 descriptor set。
        VkPushConstantRange push{
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Draw)};
        VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pl.setLayoutCount = 1;
        pl.pSetLayouts = &setLayout;
        pl.pushConstantRangeCount = 1;
        pl.pPushConstantRanges = &push;
        vkCheck(vkCreatePipelineLayout(c.device, &pl, nullptr, &pipelineLayout));
        VkShaderModule vs = shaderModule(c.device,
                                         shaders /
                                             (mode == 3 ? "triangle.vert.spv" : "scene.vert.spv")),
                       ps = shaderModule(c.device, shaders / "scene.frag.spv");
        VkPipelineShaderStageCreateInfo stages[2]{};
        for (auto& s : stages) {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            s.pName = "main";
        }
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vs;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = ps;
        VkVertexInputBindingDescription bind{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attributes[3] = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
                                                           {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12},
                                                           {2, 0, VK_FORMAT_R32G32_SFLOAT, 24}};
        VkPipelineVertexInputStateCreateInfo vi{
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        if (mode >= 4) {
            vi.vertexBindingDescriptionCount = 1;
            vi.pVertexBindingDescriptions = &bind;
            vi.vertexAttributeDescriptionCount = 3;
            vi.pVertexAttributeDescriptions = attributes;
        }
        VkPipelineInputAssemblyStateCreateInfo ia{
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount = viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo raster{
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = cull;
        raster.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo ms{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        ms.rasterizationSamples = samples;
        VkPipelineDepthStencilStateCreateInfo depth{
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depth.depthTestEnable = depth.depthWriteEnable = mode >= 5;
        depth.depthCompareOp = VK_COMPARE_OP_LESS;
        VkPipelineColorBlendAttachmentState attachment{};
        attachment.colorWriteMask =
            redOnly ? VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT : 15;
        VkPipelineColorBlendStateCreateInfo blend{
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount = 1;
        blend.pAttachments = &attachment;
        VkDynamicState ds[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = ds;

        // 动态渲染不创建 render pass，但管线仍需声明颜色和深度附件格式。
        VkPipelineRenderingCreateInfo rendering{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        rendering.colorAttachmentCount = 1;
        rendering.pColorAttachmentFormats = &colorFormat;
        rendering.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
        VkGraphicsPipelineCreateInfo gp{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        gp.pNext = &rendering;
        gp.stageCount = 2;
        gp.pStages = stages;
        gp.pVertexInputState = &vi;
        gp.pInputAssemblyState = &ia;
        gp.pViewportState = &viewport;
        gp.pRasterizationState = &raster;
        gp.pMultisampleState = &ms;
        gp.pDepthStencilState = &depth;
        gp.pColorBlendState = &blend;
        gp.pDynamicState = &dynamic;
        gp.layout = pipelineLayout;
        VkResult result = vkCreateGraphicsPipelines(c.device, cache, 1, &gp, nullptr, &pipeline);
        vkDestroyShaderModule(c.device, vs, nullptr);
        vkDestroyShaderModule(c.device, ps, nullptr);
        vkCheck(result, "Graphics pipeline");
    }

    // 这里只录制绘制指令；外层调用者负责开始/结束渲染、附件布局和提交同步。
    void render(VkCommandBuffer cmd, VkExtent2D size, float time, float roughnessScale = 1) {
        auto f = overrideFrame.value_or(frame(mode, float(size.width) / size.height, true));
        if (frameProvider)
            f = frameProvider(float(size.width) / size.height, time, true);
        uniform.upload(&f, sizeof(f));
        VkViewport viewport{
            0, 0, float(size.width) / (halfViewport ? 2 : 1), float(size.height), 0, 1};
        VkRect2D scissor{{0, 0}, size};
        if (halfScissor)
            scissor.extent.width /= 2;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptor, 0, nullptr);
        if (mode >= 4) {
            VkBuffer vb = overrideVertices ? overrideVertices : vertices.buffer;
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &offset);
            if (indices.buffer)
                vkCmdBindIndexBuffer(cmd, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
        }
        for (auto d : overrideDraws.empty() ? draws(mode, time) : overrideDraws) {
            d.settings.z = std::clamp(d.settings.z * roughnessScale, .045f, 1.f);
            vkCmdPushConstants(cmd,
                               pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(d),
                               &d);
            if (indirectArguments)
                // 绘制消费此前绑定的管线、顶点与资源；布局和资源状态必须已满足。
                vkCmdDrawIndirect(cmd, indirectArguments, 0, 1, sizeof(VkDrawIndirectCommand));
            else if (mode == 3)
                vkCmdDraw(cmd, 3, 1, 0, 0);
            else if (mode == 4)
                vkCmdDraw(cmd, uint32_t(mesh.vertices.size()), 1, 0, 0);
            else
                vkCmdDrawIndexed(cmd, uint32_t(mesh.indices.size()), mode == 7 ? 25 : 1, 0, 0, 0);
        }
    }
    ~SceneVk() {
        if (pipeline)
            vkDestroyPipeline(c.device, pipeline, nullptr);
        if (pipelineLayout)
            vkDestroyPipelineLayout(c.device, pipelineLayout, nullptr);
        if (descriptorPool)
            vkDestroyDescriptorPool(c.device, descriptorPool, nullptr);
        if (setLayout)
            vkDestroyDescriptorSetLayout(c.device, setLayout, nullptr);
        if (sampler)
            vkDestroySampler(c.device, sampler, nullptr);
    }
};

// 计算课通过 storage buffer 写入 256 个结果，再在 CPU 上逐项核对。
inline void computeSquares(const VkContextRef& c, const std::filesystem::path& shaders) {
    VkBufferResource storage;
    storage.init(c, 256 * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    VkDescriptorSetLayoutBinding binding{
        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    li.bindingCount = 1;
    li.pBindings = &binding;
    VkDescriptorSetLayout layout;
    vkCheck(vkCreateDescriptorSetLayout(c.device, &li, nullptr, &layout));
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pl.setLayoutCount = 1;
    pl.pSetLayouts = &layout;
    VkPipelineLayout pipelineLayout;
    vkCheck(vkCreatePipelineLayout(c.device, &pl, nullptr, &pipelineLayout));
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = 1;
    pool.poolSizeCount = 1;
    pool.pPoolSizes = &size;
    VkDescriptorPool descriptorPool;
    vkCheck(vkCreateDescriptorPool(c.device, &pool, nullptr, &descriptorPool));
    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    ai.descriptorPool = descriptorPool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts = &layout;
    VkDescriptorSet set;
    vkCheck(vkAllocateDescriptorSets(c.device, &ai, &set));
    VkDescriptorBufferInfo bi{storage.buffer, 0, 256 * 4};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = set;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &bi;
    vkUpdateDescriptorSets(c.device, 1, &write, 0, nullptr);
    VkShaderModule cs = shaderModule(c.device, shaders / "square.comp.spv");
    VkComputePipelineCreateInfo ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    ci.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    ci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    ci.stage.module = cs;
    ci.stage.pName = "main";
    ci.layout = pipelineLayout;
    VkPipeline pipeline;
    vkCheck(vkCreateComputePipelines(c.device, {}, 1, &ci, nullptr, &pipeline));
    immediate(c, [&](VkCommandBuffer cmd) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &set, 0, nullptr);

        // 每工作组 64 个线程，4 组覆盖 256 项；计算写入通过屏障对 host 读取可见。
        // 工作组数量向上取整；shader 必须检查尾部线程，避免越界访问。
        // Dispatch 的单位是工作组，线程组尺寸由 HLSL numthreads 声明。
        vkCmdDispatch(cmd, 4, 1, 1);
        VkMemoryBarrier2 b{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
        b.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        b.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
        b.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
        b.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
        VkDependencyInfo d{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        d.memoryBarrierCount = 1;
        d.pMemoryBarriers = &b;
        vkCmdPipelineBarrier2(cmd, &d);
    });
    void* mapped;
    vkCheck(vkMapMemory(c.device, storage.memory, 0, storage.size, 0, &mapped));
    for (uint32_t i = 0; i < 256; ++i)
        if (static_cast<uint32_t*>(mapped)[i] != i * i + 1)
            throw std::runtime_error("Compute result mismatch");
    vkUnmapMemory(c.device, storage.memory);
    std::cout << "Verified all 256 GPU compute results\n";
    vkDestroyPipeline(c.device, pipeline, nullptr);
    vkDestroyShaderModule(c.device, cs, nullptr);
    vkDestroyDescriptorPool(c.device, descriptorPool, nullptr);
    vkDestroyPipelineLayout(c.device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(c.device, layout, nullptr);
}
} // namespace course
