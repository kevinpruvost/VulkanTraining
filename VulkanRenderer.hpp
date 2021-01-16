#pragma once

// Project includes
#include "Mesh.hpp"
#include "VulkanValidation.hpp"

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
    void draw();
    void destroy();

private:
    GLFWwindow * __window;

    std::vector<Mesh> meshList;

    int currentFrame = 0;

    // Vulkan Functions
    // - Create Functions
    void createInstance();
    void createLogicalDevice();
    void createSurface();
    void createSwapChain();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSynchronisation();

    // - Validation Functions
    void setupDebugMessenger();

    // - Record Functions
    void recordCommands();

    // - Get Functions
    void getPhysicalDevice();

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