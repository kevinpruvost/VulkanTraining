#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Utilities.hpp"

#include <stdexcept>
#include <vector>
#include <set>
#include <cstring>
#include <limits>

class VulkanRenderer
{
public:
    VulkanRenderer();
    ~VulkanRenderer();

    int init(GLFWwindow * newWindow);
    void destroy();

private:
    GLFWwindow * __window;

    // Vulkan Functions
    // - Create Functions
    void createInstance();
    void createLogicalDevice();
    void createSurface();
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void createSwapChain();

    // - Destroy Functions
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

    // - Validation Functions
    void setupDebugMessenger();

    // - Get Functions
    void getPhysicalDevice();

    // - Support Functions
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
    VkInstance __instance;
    struct {
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
    } mainDevice;
    VkQueue graphicsQueue;
    VkQueue presentationQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;

    // - Validation Attributes
    VkDebugUtilsMessengerEXT debugMessenger;
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    #ifdef VULKAN_DEBUG
        const bool enableValidationLayers = true;
    #else
        const bool enableValidationLayers = false;
    #endif
};