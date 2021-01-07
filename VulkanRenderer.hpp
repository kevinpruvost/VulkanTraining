#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Utilities.hpp"

#include <stdexcept>
#include <vector>
#include <cstring>


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

    // - Get Functions
    void getPhysicalDevice();

    // - Support Functions
    // -- Checker Functions
    bool checkInstanceExtensionSupport(std::vector<const char *> * checkExtensions);
    bool checkDeviceSuitable(VkPhysicalDevice device);

    // -- Getter Functions
    QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);

    // Vulkan Components
    VkInstance __instance;
    struct {
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
    } mainDevice;
    VkQueue graphicsQueue;
};