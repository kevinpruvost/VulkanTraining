#include "VulkanRenderer.hpp"

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
        getPhysicalDevice();
    } catch (const std::runtime_error & e) {
        printf("ERROR : %s\n", e.what());
        return EXIT_FAILURE;
    }
}

void VulkanRenderer::destroy()
{
    vkDestroyInstance(__instance, nullptr);
    vkDestroyDevice(mainDevice.logicalDevice, nullptr);
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

    if (!checkInstanceExtensionSupport(&instanceExtensions))
    {
        throw std::runtime_error("VKInstance does not support required extensions !");
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    // TODO: Set up Validation Layers that Instance will use.
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;

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

    // Queue the logical device needs to create and info to do so
    // (only 1 for now, will add more later !)
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;  // Index of the family
    queueCreateInfo.queueCount = 1;                             // to create a queue from
    float priority = 1.0f;
    queueCreateInfo.pQueuePriorities = &priority;

    // Information to create logical device (sometimes called "device")
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;              // Number of Queue Create Info
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;  // List of queue create infos so device can create required queues
    deviceCreateInfo.enabledExtensionCount = 0;             // Number of enabled logical device extensions
    deviceCreateInfo.ppEnabledExtensionNames = nullptr;     // List of enabled device extensions

    // Physical Device Features the Logical Device will be using.
    VkPhysicalDeviceFeatures deviceFeatures = {};

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;    // Physical device features logical device will use.

    VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);

    if (result == VK_SUCCESS)
        throw std::runtime_error("Failed to create a logical device.");

    // Queues are created at the same time as the device.
    // So we want to handle queues
    // From given logical device,of given queue family, of given queue index
    // (0 since only one queue), please reference in given VkQueue
    vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
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
        bool hasExtension = false;
        for (const auto & extension : extensions)
        {
            if (strcmp(checkExtension, extension.extensionName))
            {
                hasExtension = true;
                break;
            }
        }

        if (!hasExtension)
        {
            return false;
        }
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

    return indices.isValid();
}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    // Get all queue families Property infos for the given device.
    uint32_t queueFamilyCount;
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

        // Check if queue family indices are in a valid state, stop searching if so
        if (indices.isValid())
            break;

        ++i;
    }
}