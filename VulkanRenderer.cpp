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

    } catch (const std::runtime_error & e) {
        printf("ERROR : %s\n", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void VulkanRenderer::destroy()
{
    // Destruction order is important !

    for (auto image : swapChainImages)
    {
        vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
    }

    vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, nullptr);
    vkDestroySurfaceKHR(__instance, surface, nullptr);
    vkDestroyDevice(mainDevice.logicalDevice, nullptr);

    if (enableValidationLayers)
        DestroyDebugUtilsMessengerEXT(__instance, debugMessenger, nullptr);

    vkDestroyInstance(__instance, nullptr);
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

    if (!checkInstanceExtensionSupport(&instanceExtensions)) {
        throw std::runtime_error("VKInstance does not support required extensions !");
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested, but not available !");
    }

    // Adds Validation Layers to create infos.
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
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

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
    void * pUserData) {

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        // Message is important enough to show
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

VkResult VulkanRenderer::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
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

void VulkanRenderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void VulkanRenderer::setupDebugMessenger()
{
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional used for casts to give a main class
                                    // or something like that containing infos for debug

    if (CreateDebugUtilsMessengerEXT(__instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
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
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &formatCount, swapChainDetails.presentationModes.data());
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