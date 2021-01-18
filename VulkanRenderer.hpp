#pragma once

// Project includes
#include "Mesh.hpp"
#include "VulkanValidation.hpp"

#include <glm/gtc/matrix_transform.hpp>

// C++ includes
#include <stdexcept>
#include <vector>
#include <set>
#include <cstring>
#include <limits>
#include <array>

class VulkanRenderer
{
public:
    VulkanRenderer();
    ~VulkanRenderer();

    int init(GLFWwindow * newWindow);

    void updateModel(int modelId, glm::mat4 newModel);

    void draw();
    void destroy();

private:
    GLFWwindow * __window;

    // Scene objects
    std::vector<Mesh> meshList;

    // Scene settings
    struct UboViewProjection {
        glm::mat4 projection;
        glm::mat4 view;
    } uboViewProjection;

    int currentFrame = 0;

    // Vulkan Functions
    // - Create Functions
    void createInstance();
    void createLogicalDevice();
    void createSurface();
    void createSwapChain();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSynchronisation();

    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();

    void updateUniformBuffers(uint32_t imageIndex);

    // - Validation Functions
    void setupDebugMessenger();

    // - Record Functions
    void recordCommands();

    // - Get Functions
    void getPhysicalDevice();

    // - Allocate Functions
    void allocateDynamicBufferTransferSpace();

    // - Support Functions
    // -- Create Functions
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkShaderModule createShaderModule(const std::vector<char> & code);

    // -- Checker Functions
    bool checkInstanceExtensionSupport(std::vector<const char *> * checkExtensions);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool checkDeviceSuitable(VkPhysicalDevice device);
    bool checkValidationLayerSupport();

    // -- Getter Functions
    QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
    SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);

    // -- Choose Functions
    VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> & formats);
    VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR> & presentationModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR & surfaceCapabilities);

    // Vulkan Components
    // - Main
    VkInstance __instance;
    VkDebugReportCallbackEXT callback;
    struct {
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
    } mainDevice;
    VkQueue graphicsQueue;
    VkQueue presentationQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;

    std::vector<SwapChainImage> swapChainImages;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    // - Descriptors
    VkDescriptorSetLayout descriptorSetLayout;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkBuffer> vpUniformBuffer;
    std::vector<VkDeviceMemory> vpUniformBufferMemory;

    std::vector<VkBuffer> modelDynamicUniformBuffer;
    std::vector<VkDeviceMemory> modelDynamicUniformBufferMemory;

    VkDeviceSize minUniformBufferOffset;
    size_t modelUniformAlignment;
    UboModel * modelTransferSpace;

    // - Pipeline
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;

    // - Pools
    VkCommandPool graphicsCommandPool;

    // - Utility
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    // - Synchronization
    std::vector<VkSemaphore> imageAvailable;
    std::vector<VkSemaphore> renderFinished;
    std::vector<VkFence> drawFences;

    // - Validation Attributes
    //#ifdef VULKAN_DEBUG
        const bool enableValidationLayers = true;
    //#else
    //    const bool enableValidationLayers = false;
    //#endif
};