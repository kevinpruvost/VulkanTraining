#include "VulkanRenderer.hpp"

// C++ includes
#include <iostream>

VulkanRenderer::VulkanRenderer()
{

}

VulkanRenderer::~VulkanRenderer()
{
    destroy();
}

int VulkanRenderer::init(GLFWwindow * newWindow)
{
    __window = newWindow;

    try {
        createInstance();

        setupDebugMessenger();

        createSurface();
        getPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();

        mvp.projection = glm::perspective(glm::radians(45.0f), (float)swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f);
        mvp.view = glm::lookAt(glm::vec3(3.0f, 1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        mvp.model = glm::mat4(1.0f);

        mvp.projection[1][1] *= -1; glm::perspective(glm::radians(45.0f), (float)swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f);

        // Create a mesh
        // Vertex Data
        std::vector<Vertex> meshVertices = {
            { { -0.1, -0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },	// 0
            { { -0.1, 0.4, 0.0 },{ 0.0f, 1.0f, 0.0f } },	    // 1
            { { -0.9, 0.4, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
            { { -0.9, -0.4, 0.0 },{ 1.0f, 1.0f, 0.0f } },   // 3
        };

        std::vector<Vertex> meshVertices2 = {
            { { 0.9, -0.3, 0.0 },{ 1.0f, 0.0f, 0.0f } },	// 0
            { { 0.9, 0.1, 0.0 },{ 0.0f, 1.0f, 0.0f } },	    // 1
            { { 0.1, 0.3, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
            { { 0.1, -0.3, 0.0 },{ 1.0f, 1.0f, 0.0f } },   // 3
        };

        // Index Data
        std::vector<uint32_t> meshIndices = {
            0, 1, 2,
            2, 3, 0
        };

        Mesh firstMesh = Mesh(mainDevice.physicalDevice, mainDevice.logicalDevice,
            graphicsQueue, graphicsCommandPool,
            &meshVertices, &meshIndices);
        Mesh secondMesh = Mesh(mainDevice.physicalDevice, mainDevice.logicalDevice,
            graphicsQueue, graphicsCommandPool,
            &meshVertices2, &meshIndices);

        meshList.push_back(firstMesh);
        meshList.push_back(secondMesh);

        createCommandBuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        recordCommands();
        createSynchronisation();

    } catch (const std::runtime_error & e) {
        printf("ERROR : %s\n", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void VulkanRenderer::destroy()
{
    // Destruction order is important !

    // Wait until no actions being run on device before destroying.
    vkDeviceWaitIdle(mainDevice.logicalDevice);

    vkDestroyDescriptorPool(mainDevice.logicalDevice, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, descriptorSetLayout, nullptr);
    for (size_t i = 0; i < uniformBuffer.size(); ++i)
    {
        vkDestroyBuffer(mainDevice.logicalDevice, uniformBuffer[i], nullptr);
        vkFreeMemory(mainDevice.logicalDevice, uniformBufferMemory[i], nullptr);
    }
    for (size_t i = 0; i < meshList.size(); i++)
    {
        meshList[i].destroyVertexBuffer();
    }

    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        vkDestroySemaphore(mainDevice.logicalDevice, renderFinished[i], nullptr);
        vkDestroySemaphore(mainDevice.logicalDevice, imageAvailable[i], nullptr);
        vkDestroyFence(mainDevice.logicalDevice, drawFences[i], nullptr);
    }

    vkDestroyCommandPool(mainDevice.logicalDevice, graphicsCommandPool, nullptr);

    for (auto& framebuffer : swapChainFramebuffers)
    {
        vkDestroyFramebuffer(mainDevice.logicalDevice, framebuffer, nullptr);
    }

    vkDestroyPipeline(mainDevice.logicalDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(mainDevice.logicalDevice, pipelineLayout, nullptr);
    vkDestroyRenderPass(mainDevice.logicalDevice, renderPass, nullptr);

    for (auto image : swapChainImages)
    {
        vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
    }

    vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, nullptr);
    vkDestroySurfaceKHR(__instance, surface, nullptr);
    vkDestroyDevice(mainDevice.logicalDevice, nullptr);

    if (enableValidationLayers)
        DestroyDebugReportCallbackEXT(__instance, callback, nullptr);

    vkDestroyInstance(__instance, nullptr);
}

void VulkanRenderer::draw()
{
    // -- GET NEXT IMAGE --
    // Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
    uint32_t imageIndex;
    vkAcquireNextImageKHR(mainDevice.logicalDevice, swapchain,
        std::numeric_limits<uint64_t>::max(), imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);

	// Wait for given fence to signal (open) from last draw before continuing
	vkWaitForFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	// Manually reset (close) fences
	vkResetFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame]);

    updateUniformBuffer(imageIndex);

    // -- SUBMIT COMMAND BUFFER TO RENDER --
    // Queue submission information
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;                      // Number of semaphores to wait on
    submitInfo.pWaitSemaphores = &imageAvailable[currentFrame];           // List of semaphores to wait on
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submitInfo.pWaitDstStageMask = waitStages;                  // Stages to check semaphores at
    submitInfo.commandBufferCount = 1;                          // Number of commands to submit
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];   // Command buffer to submit
    submitInfo.signalSemaphoreCount = 1;                        // Number of semaphores to signal
    submitInfo.pSignalSemaphores = &renderFinished[currentFrame];             // Semaphores to signal when command buffer finishes

    // Submit command buffer to queue.
    VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit Command Buffer to Queue !");
    }

    // -- PRESENT RENDERED IMAGE TO SCREEN --
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;             // Number of semaphores to wait on
    presentInfo.pWaitSemaphores = &renderFinished[currentFrame];  // Semaphores to wait on
    presentInfo.swapchainCount = 1;                 // Number of swapchains to present to
    presentInfo.pSwapchains = &swapchain;           // Swapchains to present images to
    presentInfo.pImageIndices = &imageIndex;        // Index of images in swapchains to present

    // Present image
    result = vkQueuePresentKHR(presentationQueue, &presentInfo);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present image !");
    }

    // Get next frame
    currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
}

void VulkanRenderer::createInstance()
{
    // Infos about the app itself.
    // Most data here doesn't affect the program and is for developer convenience.
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan App";                    // Custom name
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);      // Custom version
    appInfo.pEngineName = "No Engine";                          // Custom engine name
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);           // Custom engine version
    appInfo.apiVersion = VK_API_VERSION_1_0;                    // /!\ Vulkan version !

    // Creation infos for a VKInstance.
    VkInstanceCreateInfo createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Create list to hold instance extensions.
    std::vector<const char *> instanceExtensions;

    // Setup extension Instance will use
    uint32_t glfwExtensionCount = 0;            // GLFW may require multiple extensions.
    const char ** glfwExtensions;               // Extensions passed as array of cstrings.

    // Get GLFW extensions.
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Add GLFW extensions to list of extensions.
    for (size_t i = 0; i < glfwExtensionCount; ++i)
        instanceExtensions.push_back(glfwExtensions[i]);

    // If validation enabled, add extension to report validation debug info
    if (enableValidationLayers)
    {
        instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    if (!checkInstanceExtensionSupport(&instanceExtensions)) {
        throw std::runtime_error("VKInstance does not support required extensions !");
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested, but not available !");
    }

    // Adds Validation Layers to create infos.
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &__instance);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Vulkan Instance !");
    }
}

void VulkanRenderer::createLogicalDevice()
{
    // Get the queue family for the chosen physical device.
    QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

    // Queue the logical device needs to create and info to do so
    // (only 1 for now, will add more later !)
    for (int queueFamilyIndex : queueFamilyIndices)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;  // Index of the family
        queueCreateInfo.queueCount = 1;                       // to create a queue from
        float priority = 1.0f;
        queueCreateInfo.pQueuePriorities = &priority;

		queueCreateInfos.push_back(queueCreateInfo);
    }

    // Information to create logical device (sometimes called "device")
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());              // Number of Queue Create Info
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();  // List of queue create infos so device can create required queues
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()); // Number of enabled logical device extensions
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();     // List of enabled device extensions

    // Physical Device Features the Logical Device will be using.
    VkPhysicalDeviceFeatures deviceFeatures = {};
    //deviceFeatures.depthClamp = VK_TRUE;

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;    // Physical device features logical device will use.

    VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);

    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a logical device.");

    // Queues are created at the same time as the device.
    // So we want to handle queues
    // From given logical device,of given queue family, of given queue index
    // (0 since only one queue), please reference in given VkQueue
    vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(mainDevice.logicalDevice, indices.presentationFamily, 0, &presentationQueue);
}

void VulkanRenderer::createSurface()
{
    // Create Surface (creates a surface create info struct, runs the create
    // surface function, returns result)
    VkResult result = glfwCreateWindowSurface(__instance, __window, nullptr, &surface);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a surface !");
    }
}

void VulkanRenderer::createSwapChain()
{
    // Get Swap Chain Details so we can pick best settings
    SwapChainDetails swapChainDetails = getSwapChainDetails(mainDevice.physicalDevice);

    // 1. Choose best surface format.
    VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
    // 2. Choose best presentation mode.
    VkPresentModeKHR presentMode = chooseBestPresentationMode(swapChainDetails.presentationModes);
    // 3. Choose swap chain image resolution.
    VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

    // How many images are in the swap chain ? Get 1 more than the minimum to
    // allow triple buffering.
    uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

    // If image count higher than max, then clamp it.
    // It max == 0, then limitless
    if (swapChainDetails.surfaceCapabilities.maxImageCount > 0)
        imageCount = std::min(imageCount, swapChainDetails.surfaceCapabilities.maxImageCount);

    // Create information for swap chain
    VkSwapchainCreateInfoKHR swapChainCreateInfos = {};
    swapChainCreateInfos.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfos.surface = surface;                             // Swapchain surface.
    swapChainCreateInfos.imageFormat = surfaceFormat.format;            // Swapchain format
    swapChainCreateInfos.imageColorSpace = surfaceFormat.colorSpace;    // Swapchain color space
    swapChainCreateInfos.presentMode = presentMode;                     // Swapchain presentation mode
    swapChainCreateInfos.imageExtent = extent;                          // Swapchain image extent
    swapChainCreateInfos.minImageCount = imageCount;                    // Minimum images in swapchain
    swapChainCreateInfos.imageArrayLayers = 1;                          // Number of layers for each image in chain
    swapChainCreateInfos.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  // What attachment images will be used as
    swapChainCreateInfos.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;  // Transform to perform on swap chain images.
    swapChainCreateInfos.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;    // How to handle blending images with external graphics (e.g. other windows).
    swapChainCreateInfos.clipped = VK_TRUE;     // Whether to clip parts of image not in view (e.g. behind another window, off screen, etc)

    // Get Queue Family Indices
    QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

    // If Graphics and Presentation families are different, then swapchain must let
    // images be shared between families.
    if (indices.graphicsFamily != indices.presentationFamily)
    {
        uint32_t queueFamilyIndices[] = {
            static_cast<uint32_t>(indices.graphicsFamily),
            static_cast<uint32_t>(indices.presentationFamily)
        };

        swapChainCreateInfos.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Image share handling
        swapChainCreateInfos.queueFamilyIndexCount = 2;                     // Number of queues to share images between
        swapChainCreateInfos.pQueueFamilyIndices = queueFamilyIndices;      // Array of queues to share between
    }
    else
    {
        swapChainCreateInfos.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfos.queueFamilyIndexCount = 0;
        swapChainCreateInfos.pQueueFamilyIndices = nullptr;
    }

    // If old swap chain been destroyed and this one replaces it, then link old one to quickly hand over responsibilities
    swapChainCreateInfos.oldSwapchain = VK_NULL_HANDLE;

    // Create Swapchain
    VkResult result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapChainCreateInfos, nullptr, &swapchain);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a swapchain !");
    }

    // Store for later references
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    uint32_t swapChainImageCount;
    vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapChainImageCount, nullptr);

    std::vector<VkImage> images(swapChainImageCount);
    vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapChainImageCount, images.data());

    for (VkImage image : images)
    {
        // Store image handle
        SwapChainImage swapChainImage = {};
        swapChainImage.image = image;

        // Create Image View
        swapChainImage.imageView = createImageView(image, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

        // Add to swapchain images list
        swapChainImages.push_back(swapChainImage);
    }
}

void VulkanRenderer::createRenderPass()
{
    // Color attachment of the render pass.
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainImageFormat;                      // Format to use for attachment
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;                    // Number of samples to write for multisampling
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;               // Describes what to do with the attachment before rendering
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;             // Describes what to do with the attachment after rendering
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;    // Describes what to do with stencil before rendering
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // Describes what to do with stencil after rendering

    // Framebuffer data will be stored as an image, but images can be given different data layouts
    // to give optimal use for certain operations
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          // Image data layout before render pass starts
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;      // Image data layout after render pass (to change to)

    // Attachment reference uses an attachement index that refers to index in the attachment list passed to renderPassCreateInfo
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Information about a particular subpass the Render Pass is using
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;        // Pipeline type subpass is to be bound to, that's where we can implement Nvidia Ray Tracing
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;

    // Need to determine when layout transitions occur using subpass dependencies
    std::array<VkSubpassDependency, 2> subpassDependencies;

    // Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    // Transition must happen after...
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;                        // Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;     // Pipeline stage
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;               // Stage access mask (memory access)
    // But must happen before...
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    subpassDependencies[0].dependencyFlags = 0;

    // Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENCE_SRC_KHR
    // Transition must happen after...
    subpassDependencies[1].srcSubpass = 0;                                                  // Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;    // Pipeline stage
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  // Stage access mask (memory access)
    // But must happen before...
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    subpassDependencies[1].dependencyFlags = 0;

    // Create render pass create info
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassCreateInfo.pDependencies = subpassDependencies.data();

    VkResult result = vkCreateRenderPass(mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Render Pass !");
    }
}

void VulkanRenderer::createDescriptorSetLayout()
{
    // MVP Binding Info
    VkDescriptorSetLayoutBinding mvpLayoutBinding = {};
    mvpLayoutBinding.binding = 0;                                           // Binding point in shader (designated by binding number in shader)
    mvpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    // Type of descriptor (uniform, dynamic uniform, image sample, etc)
    mvpLayoutBinding.descriptorCount = 1;                                   // Number of descriptors for binding
    mvpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;               // Shader stage to bind to
    mvpLayoutBinding.pImmutableSamplers = nullptr;                          // For textures: Can make data sampler unchangeable (immutable) by specifying in layout

    // Create descriptor set layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = 1;                  // Number of binding infos
    layoutCreateInfo.pBindings = &mvpLayoutBinding;     // Array of binding infos

    // Create Descriptor Set Layout
    VkResult result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a descriptor set layout !");
    }
}

void VulkanRenderer::createGraphicsPipeline()
{
    // Read in SPIR-V code of shaders
    auto vertexShaderCode = readFile("Shaders/shader1_vert.spv");
    auto fragmentShaderCode = readFile("Shaders/shader1_frag.spv");

    // Build Shader Modules to link to Graphics Pipeline
    VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

    // -- SHADER STAGE CREATION INFO --
    // Vertex Stage creation information
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;  // Shader stage name
    vertexShaderCreateInfo.module = vertexShaderModule;         // Shader module to be used by stage
    vertexShaderCreateInfo.pName = "main";                      // Entry point in the shader

    // Fragment Stage creation information
    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;  // Shader stage name
    fragmentShaderCreateInfo.module = fragmentShaderModule;         // Shader module to be used by stage
    fragmentShaderCreateInfo.pName = "main";                      // Entry point in the shader

    // Put shader stage creation info into array
    // Graphics Pipeline creation info requires array of shader stage creates
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

    // How the data for a single vertex (including info such as position, color, texture coords, normals, etc) is as a whole
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;                             // Can bind multiple streams of data
    bindingDescription.stride = sizeof(Vertex);                 // Size of a single vertex object
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // How to move between data after each vertex
                                                                // VK_VERTEX_INPUT_RATE_INDEX       : Move on to the next vertex
                                                                // VK_VERTEX_INPUT_RATE_DISTANCE    : Move to a vertex for the next instance
    
    // How the data for an attribute is defined within a vertex
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;

    // Position attribute
    attributeDescriptions[0].binding = 0;           // Which binding the data is at (should be the same as above)
    attributeDescriptions[0].location = 0;          // Location in shader where data will be read from
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;    // Format the data will take (also helps defined size of data)
    attributeDescriptions[0].offset = offsetof(Vertex, pos);            // Where this attribute is defined in the data for a single vertex

    // Color attribute 
    attributeDescriptions[1].binding = 0;           // Which binding the data is at (should be the same as above)
    attributeDescriptions[1].location = 1;          // Location in shader where data will be read from
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;    // Format the data will take (also helps defined size of data)
    attributeDescriptions[1].offset = offsetof(Vertex, col);            // Where this attribute is defined in the data for a single vertex

    // -- Vertex input (TODO: Put in vertex descriptions when resources created) --
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;              // List of Vertex Binding Descriptions (data spacing / stride info)
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();   // List of Vertex Attribute Descriptions (data format and where to bind/to or from)

    // -- Input assembly --
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;       // Primitive type to assemble vertices as
    inputAssembly.primitiveRestartEnable = VK_FALSE;                    // Allow overriding of "strip" topology to start new primitives

    // -- Viewport & Scissor --
    // Create a viewport info struct
    VkViewport viewport = {};
    viewport.x = 0.0f;      // X start coordinates
    viewport.y = 0.0f;      // Y start coordinates
    viewport.width = static_cast<float>(swapChainExtent.width);     // width of viewport
    viewport.height = static_cast<float>(swapChainExtent.height);   // height of viewport
    viewport.minDepth = 0.0f;   // min framebuffer depth
    viewport.maxDepth = 1.0f;   // max framebuffer depth

    // Create a scissor info struct
    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };          // Offset to use region from
    scissor.extent = swapChainExtent;   // Extent to describe region to use, starting at offset

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    // -- Dynamic states --
    // Dynamic states to enable
    std::vector<VkDynamicState> dynamicStateEnables;
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);   // Dynamic viewport : Can resize in command buffer with vkCmdSetViewport(commandBuffer, 0, 1, &viewport)
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);    // Dynamic Scissor  : Can resize in command buffer with vkCmdSetScissor(commandBuffer, 0, 1, &viewport)

    // Dynamic States creation info
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();

    // -- Rasterizer --
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE; // = VK_TRUE;         // Change if fragments beyoond near/far planes are clipped (default) or clamped to plane
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;                // Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output.
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;                // How to handle filling points between vertices
    rasterizerCreateInfo.lineWidth = 1.0f;                                  // How thick lines should be when drawn
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;                  // Which face of a tri to cull
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;       // Winding to determine which size is front
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;                        // Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)

    // -- Multi sampling --
    VkPipelineMultisampleStateCreateInfo multiSamplingCreateInfo = {};
    multiSamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multiSamplingCreateInfo.sampleShadingEnable = VK_FALSE;                  // Enable multisample shading or not.
    multiSamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;   // Number of samples to use per fragment.

    // -- Blending --
    // Blending decides how to blend a new color being written to a fragment, with the old value

    // Blend attachment state (how blending is handled)
    VkPipelineColorBlendAttachmentState colorState = {};
    colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT         // Colors to apply landing to
                              | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorState.blendEnable = VK_TRUE;               // Enable blending

    // Blending uses equation : (srcColorBlendFactor * new color) colorBlendOp (dstColorBlendFactor * old color)
    colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorState.colorBlendOp = VK_BLEND_OP_ADD;

    // Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new color) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old color)
    //             (new color alpha * new color) + ((1 - new color alpha) * old color)

    colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorState.alphaBlendOp = VK_BLEND_OP_ADD;
    // Summarised: (1 * new alpha) + (0 * old alpha) = new alpha

    VkPipelineColorBlendStateCreateInfo colorBlendingStateCreateInfo = {};
    colorBlendingStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingStateCreateInfo.logicOpEnable = VK_FALSE;              // Alternative to calculations is to use logical operations
    colorBlendingStateCreateInfo.attachmentCount = 1;
    colorBlendingStateCreateInfo.pAttachments = &colorState;

    // -- Pipeline layout --
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    // Create Pipeline Layout
    VkResult result = vkCreatePipelineLayout(mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a pipeline layout !");
    }

    // -- Depth Stencil Testing
    // TODO: Set up depth stencil testing



    // -- Graphics Pipeline Creation --
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;                                      // Number of shader stages
    pipelineCreateInfo.pStages = shaderStages;                              // List of shader stages
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;          // All the fixed function pipeline states
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;// &dynamicStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multiSamplingCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendingStateCreateInfo;
    pipelineCreateInfo.pDepthStencilState = nullptr;
    pipelineCreateInfo.layout = pipelineLayout;                             // Pipeline layout pipeline should use
    pipelineCreateInfo.renderPass = renderPass;                             // Render Pass description the pipeline is compatible with
    pipelineCreateInfo.subpass = 0;                                         // Subpass of render pass to use with pipeline

    // Pipeline Derivatives : Can Create multiple pipelines that derive from one 
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Existing pipeline to derive from...
    pipelineCreateInfo.basePipelineIndex = -1;              // or index of pipeline being created to derive from (in case creating multiple at once)

    // Create graphics pipeline
    result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Graphics Pipeline !");
    }

    // Destroy shader modules, no longer needed after Pipeline created
    vkDestroyShaderModule(mainDevice.logicalDevice, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(mainDevice.logicalDevice, vertexShaderModule, nullptr);
}

void VulkanRenderer::createFramebuffers()
{
    // Resize framebuffer count to equal swap chain image count
    swapChainFramebuffers.resize(swapChainImages.size());

    // Create a framebuffer for each swap chain image
    for (size_t i = 0; i < swapChainFramebuffers.size(); ++i)
    {
        std::array<VkImageView, 1> attachments = {
            swapChainImages[i].imageView
        };

        VkFramebufferCreateInfo frameBufferCreateInfo = {};
        frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCreateInfo.renderPass = renderPass;                                      // Render Pass Layout the framebuffer will be used with
        frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        frameBufferCreateInfo.pAttachments = attachments.data();                            // List of attachments (1:1 with Render Pass)
        frameBufferCreateInfo.width = swapChainExtent.width;                                // Framebuffer width
        frameBufferCreateInfo.height = swapChainExtent.height;                              // Framebuffer height
        frameBufferCreateInfo.layers = 1;                                                   // Framebuffer layers

        VkResult result = vkCreateFramebuffer(mainDevice.logicalDevice, &frameBufferCreateInfo, nullptr, &swapChainFramebuffers[i]);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a framebuffer !");
        }
    }
}

void VulkanRenderer::createCommandPool()
{
    // Get indices of queue families from device
    QueueFamilyIndices queueFamilyIndices = getQueueFamilies(mainDevice.physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;  // Queue Family type that buffers from this command pool will use

    // Create a Graphics Queue Family Command Pool
    VkResult result = vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, &graphicsCommandPool);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a graphics command pool !");
    }
}

void VulkanRenderer::createCommandBuffers()
{
    // Resize command buffer count to have one for each framebuffer
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = graphicsCommandPool;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;    // VK_COMMAND_BUFFER_LEVEL_PRIMARY   : Buffer you submit directly to queue. Cant be called by other buffers
                                                            // VK_COMMAND_BUFFER_LEVEL_SECONDARY : Buffer can't be called directly, Can be called from other buffers via
                                                            //                                     "vkCmdExecuteCommands" when recording commands in primary buffer
    cbAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    VkResult result = vkAllocateCommandBuffers(mainDevice.logicalDevice, &cbAllocInfo, commandBuffers.data());

    if (result != VK_SUCCESS)
    {
        std::runtime_error("Failed to allocate Command Buffers !");
    }
}

void VulkanRenderer::createSynchronisation()
{
    imageAvailable.resize(MAX_FRAME_DRAWS);
    renderFinished.resize(MAX_FRAME_DRAWS);
	drawFences.resize(MAX_FRAME_DRAWS);

    // Semaphore creation information
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Fence creation information
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        if (vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailable[i]) != VK_SUCCESS
         || vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinished[i]) != VK_SUCCESS
         || vkCreateFence(mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &drawFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a Semaphore !");
        }
    }
}

void VulkanRenderer::createUniformBuffers()
{
    // Buffer size will be size of all three variables (will offset to access)
    VkDeviceSize bufferSize = sizeof(MVP);

    // One uniform buffer for each image (and by extension, command buffer)
    uniformBuffer.resize(swapChainImages.size());
    uniformBufferMemory.resize(swapChainImages.size());

    // Create Uniform buffers
    for (size_t i = 0; i < swapChainImages.size(); ++i)
    {
        createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &uniformBuffer[i], &uniformBufferMemory[i]);
    }
}

void VulkanRenderer::createDescriptorPool()
{
    // Type of descriptors + how many DESCRIPTORS, not Descriptor Sets (combined makes the pool size)
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(uniformBuffer.size());

    // Data to create Descriptor Pool
    VkDescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.maxSets = static_cast<uint32_t>(uniformBuffer.size());   // Maximum number of descriptor sets that can be created from pool
    poolCreateInfo.poolSizeCount = 1;                                       // Amount of Pools sizes being passed
    poolCreateInfo.pPoolSizes = &poolSize;                                  // Sizes to create pool with

    VkResult result = vkCreateDescriptorPool(mainDevice.logicalDevice, &poolCreateInfo, nullptr, &descriptorPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Descriptor Pool !");
    }
}

void VulkanRenderer::createDescriptorSets()
{
    // Resize Descriptor Set list so one for every buffer
    descriptorSets.resize(uniformBuffer.size());

    std::vector<VkDescriptorSetLayout> setLayouts(uniformBuffer.size(), descriptorSetLayout);

    // Descriptor Set Allocation Info
    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = descriptorPool;                                   // Pool to allocate descriptor set from
    setAllocInfo.descriptorSetCount = static_cast<uint32_t>(uniformBuffer.size());  // Number of sets to allocate
    setAllocInfo.pSetLayouts = setLayouts.data();                                   // Layouts to use to allocate sets (1:1 relationship)
    
    // Allocate descriptor sets (multiple)
    VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &setAllocInfo, descriptorSets.data());
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to Allocate Descriptor Sets !");
    }

    // Update all of descriptor set buffer bindings
    for (size_t i = 0; i < uniformBuffer.size(); ++i)
    {
        // Buffer info and data offset info
        VkDescriptorBufferInfo mvpBufferInfo = {};
        mvpBufferInfo.buffer = uniformBuffer[i];    // Buffer to get data from
        mvpBufferInfo.offset = 0;                   // Position of start of data
        mvpBufferInfo.range = sizeof(MVP);          // Size of data

        // Data about connection between binding and buffer
        VkWriteDescriptorSet mvpSetWrite = {};
        mvpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        mvpSetWrite.dstSet = descriptorSets[i];                             // Description Set to update
        mvpSetWrite.dstBinding = 0;                                         // Binding to update (matches with binding on layout/shader)
        mvpSetWrite.dstArrayElement = 0;                                    // Index in array to update
        mvpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;     // Type of descriptor
        mvpSetWrite.descriptorCount = 1;                                    // Amount to update
        mvpSetWrite.pBufferInfo = &mvpBufferInfo;                           // Information about buffer data to bind

        // Update descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets(mainDevice.logicalDevice, 1, &mvpSetWrite, 0, nullptr);
    }
}

void VulkanRenderer::updateUniformBuffer(uint32_t imageIndex)
{
    void * data;
    vkMapMemory(mainDevice.logicalDevice, uniformBufferMemory[imageIndex], 0, sizeof(MVP), 0, &data);
    memcpy(data, &mvp, sizeof(MVP));
    vkUnmapMemory(mainDevice.logicalDevice, uniformBufferMemory[imageIndex]);
}

void VulkanRenderer::setupDebugMessenger()
{
    if (!enableValidationLayers) return;

    VkDebugReportCallbackCreateInfoEXT callbackCreateInfo = {};
    callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;	// Which validation reports should initiate callback
    callbackCreateInfo.pfnCallback = debugCallback;												// Pointer to callback function itself

    // Create debug callback with custom create function
    PFN_vkCreateDebugReportCallbackEXT createDebugReportCallback = VK_NULL_HANDLE;
    createDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(__instance, "vkCreateDebugReportCallbackEXT");
    VkResult result = createDebugReportCallback(__instance, &callbackCreateInfo, nullptr, &callback);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Debug Callback!");
    }
}

void VulkanRenderer::recordCommands()
{
    // Information about how to begin each command buffer
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;   // Buffer can be resubmitted when it has already been submitted

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;                            // Render Pass to begin
    renderPassBeginInfo.renderArea.offset = { 0, 0 };                       // Start point of render pass in pixels
    renderPassBeginInfo.renderArea.extent = swapChainExtent;                // Size of region to render pass on (starting at offset)
    VkClearValue clearValues[] = {
        {0.6f, 0.65f, 0.4f, 1.0f}
    };
    renderPassBeginInfo.pClearValues = clearValues;                         // List of clear values (TODO: Depth Attachment Clear Value)
    renderPassBeginInfo.clearValueCount = 1;


    for (size_t i = 0; i < commandBuffers.size(); ++i)
    {
        renderPassBeginInfo.framebuffer = swapChainFramebuffers[i];
        // Start recording commands to command buffer !
        VkResult result = vkBeginCommandBuffer(commandBuffers[i], &bufferBeginInfo);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to start recording a Command Buffer !");
        }

        // Begin Render Pass
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            // Bind Pipeline to be used in the render pass
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            for (size_t j = 0; j < meshList.size(); j++)
            {
                VkBuffer vertexBuffers[] = { meshList[j].getVertexBuffer() };					// Buffers to bind
                VkDeviceSize offsets[] = { 0 };												// Offsets into buffers being bound
                vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);	// Command to bind vertex buffer before drawing with them

                // Bind mesh index buffer, with 0 offset and using the uint32 type
                vkCmdBindIndexBuffer(commandBuffers[i], meshList[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

                // Bind descriptor sets
                vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                    0, 1, &descriptorSets[i], 0, nullptr);

                // Execute pipeline
                vkCmdDrawIndexed(commandBuffers[i], meshList[j].getIndexCount(), 1, 0, 0, 0);
            }
        }
        // End Render Pass
        vkCmdEndRenderPass(commandBuffers[i]);

        // Stop recording to command buffer !
        result = vkEndCommandBuffer(commandBuffers[i]);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to stop recording a Command Buffer !");
        }
    }

}

void VulkanRenderer::getPhysicalDevice()
{
    // Enumerates Physical devices the vkInstance can access.
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(__instance, &deviceCount, nullptr);

    // If no devices available, then none support Vulkan.
    if (deviceCount == 0)
        throw std::runtime_error("Can't find GPUs that support Vulkan instances.");

    // Get list of Physical devices.
    std::vector<VkPhysicalDevice> deviceList(deviceCount);
    vkEnumeratePhysicalDevices(__instance, &deviceCount, deviceList.data());

    for (const auto & device : deviceList)
    {
        if (checkDeviceSuitable(device))
        {
            mainDevice.physicalDevice = device;
            break;
        }
    }
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;                       // Image to create view for
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;    // Type of image (1D, 2D, 3D, Cube)
    viewCreateInfo.format = format;                     // Format of image data
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;    // Allows remapping of rgba
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;    // components to other
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;    // rgba values
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // Subresources allow the view to view only a part of an image
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;   // Which aspect of image to view (e.g. COLOR_BIT for viewing color)
    viewCreateInfo.subresourceRange.baseMipLevel = 0;           // Start mipmap level to view from
    viewCreateInfo.subresourceRange.levelCount = 1;             // Number of mipmap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;         // Start array level to view from
    viewCreateInfo.subresourceRange.layerCount = 1;             // Number of array levels to view

    // Create image view and return it
    VkImageView imageView;
    VkResult result = vkCreateImageView(mainDevice.logicalDevice, &viewCreateInfo,
                                        nullptr, &imageView);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create an Image View !");
    }

    return imageView;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char> & code)
{
    // Shader Module Create Info
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = code.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;

    VkResult result = vkCreateShaderModule(mainDevice.logicalDevice, &shaderModuleCreateInfo,
                                           nullptr, &shaderModule);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a shader module !");
    }

    return shaderModule;
}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char *> * checkExtensions)
{
    // Need to get number of extensions to create array of correct size to hold extensions.
    uint32_t extensionsCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);

    // Create a list of VkExtensionsProperties using count.
    std::vector<VkExtensionProperties> extensions(extensionsCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensions.data());

    // Check if given extensions are in list of available extensions
    for (const auto & checkExtension : *checkExtensions)
    {
        for (const auto & extension : extensions)
        {
            if (strcmp(checkExtension, extension.extensionName))
            {
                goto found;
            }
        }

        return false;
        found: {}
    }
    return true;
}

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    // Get device extensions count.
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    // If no extensions found, return failure.
    if (extensionCount == 0)
        return false;

    // Populate list of extensions
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

    // Check for extension
    for (const auto & deviceExtension : deviceExtensions)
    {
        for (const auto & extension : extensions)
        {
            if (strcmp(deviceExtension, extension.extensionName) == 0)
            {
                goto found;
            }
        }

        return false;
        found: {}
    }
    return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
    /*
    // Information about the device itself (ID, name, type, vendor, etc)
    VkPhysicalDeviceProperties devicePropeties;
    vkGetPhysicalDeviceProperties(device, &devicePropeties);

    // Infos about what the device can do (geo shader, tess shader, wide lines, etc)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    */

    QueueFamilyIndices indices = getQueueFamilies(device);

    if (!indices.isValid())
        return false;

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    if (!extensionsSupported)
        return false;

    bool swapChainValid = false;

    SwapChainDetails swapChainDetails = getSwapChainDetails(device);
    swapChainValid = !swapChainDetails.presentationModes.empty() &&
                     !swapChainDetails.formats.empty();

    return swapChainValid;
}

bool VulkanRenderer::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                goto found;
            }
        }

        return false;
        found: {}
    }

    return true;
}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    // Get all queue families Property infos for the given device.
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

    // Go through each queue family and check if it has at least 1 of the required
    // types of queue
    int i = 0;
    for (const auto & queueFamily : queueFamilyList)
    {
        // First check if queue family has at least 1 queue in that family (could have no queues)
        // Queue can be multiple types defined through bitfield. Need to bitwise AND with
        // VK_QUEUE_GRAPHICS_BIT to check if it has required type
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;     // If queue family is valid, then get index
        }

        // Check if queue family supports presentation.
        VkBool32 presentationSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);

        // Check if queue is presentation type (can be both graphics and presentation)
        if (queueFamily.queueCount > 0 && presentationSupport)
        {
            indices.presentationFamily = i;
        }

        // Check if queue family indices are in a valid state, stop searching if so
        if (indices.isValid())
            break;

        ++i;
    }
    return indices;
}

SwapChainDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device)
{
    SwapChainDetails swapChainDetails;

    // - Capabilities
    // Get the surface capabilities for the given surface on the physical device
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

    // - Formats
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    // If formats returned, get list of formats
    if (formatCount != 0)
    {
        swapChainDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
    }

    // - Presentation modes
    uint32_t presentationCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

    // If presentations returned, get list of presentations
    if (presentationCount != 0)
    {
        swapChainDetails.presentationModes.resize(presentationCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapChainDetails.presentationModes.data());
    }

    return swapChainDetails;
}

// Best format is subjective
// Format       : VK_FORMAT_R8G8B8A8_UNORM
// Color Space  : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> & formats)
{
    // If only 1 format available and is undefined, then this means all formats
    // are available.
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    // If restricted, search for optimal format.
    for (const auto & format : formats)
    {
        if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
          && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    // If can't find optimal format, then return the first.
    return formats[0];
}

VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR> & presentationModes)
{
    // Look for mailbox presentation modes.
    for (const auto & presentationMode : presentationModes)
    {
        if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return presentationMode;
    }

    // If can't find, use FIFO as Vulkan spec says it must be present.
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & surfaceCapabilities)
{
    // If current extent is at numeric limits, then extent can vary. Otherwise,
    // it is the size of the window.
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return surfaceCapabilities.currentExtent;
    }
    else
    {
        // If value can vary, need to set manually.

        // Get window size.
        int width, height;
        glfwGetFramebufferSize(__window, &width, &height);

        // Create new extent using window size.
        VkExtent2D newExtent = {};
        newExtent.width = static_cast<uint32_t>(width);
        newExtent.height = static_cast<uint32_t>(height);

        // Surface also defines max and min, so make sure within boundaries by
        // clamping values.
        newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
        newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

        return newExtent;
    }
}