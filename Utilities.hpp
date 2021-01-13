#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <vector>
#include <fstream>

const int MAX_FRAME_DRAWS = 2;

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct Vertex
{
    glm::vec3 pos; // Vertex position (x, y, z)
    glm::vec3 col; // Vertex Colour (r, g, b)
};

// Indices (locations) of Queue Families (if they exist at all)
struct QueueFamilyIndices {
    int graphicsFamily = -1;        // Location of Graphics Queue Family
    int presentationFamily = -1;

    // Check if queue families are valid.
    bool isValid()
    {
        return graphicsFamily >= 0 && presentationFamily >= 0;
    }
};

struct SwapChainDetails {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;   // Surface properties, e.g. image size/extent
    std::vector<VkSurfaceFormatKHR> formats;        // Surface image formats, e.g. RGBA and size of each color
    std::vector<VkPresentModeKHR> presentationModes; // How images should be presented.
};

struct SwapChainImage {
    VkImage image;
    VkImageView imageView;
};

static std::vector<char> readFile(const std::string & fileName)
{
    // Open stream from given file.
    // std::ios::binary tells to read it as binaries
    // std::ios::ate tells to start reading from the end of the file
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);

    // Check if file stream successfully opened
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open a file !");
    }

    // Get current read position and use to resize file buffer.
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> fileBuffer(fileSize);

    // Move read position to the start of the file
    file.seekg(0);

    // Read the file data into the buffer (stream "filesize" in total)
    file.read(fileBuffer.data(), fileSize);

    // Close stream
    file.close();

    return fileBuffer;
}

static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
    // Get properties of physical device memory
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        // Index of memory type must match corresponding bit in allowedTypes
        // && desired properties bit flags are part of memory type's property flags
        if (((allowedTypes) & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            // This memory type is valid, so return the index
            return i;
        }
    }
}

static void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags,
                         VkMemoryPropertyFlags bufferProperties, VkBuffer * buffer, VkDeviceMemory * memory)
{
    // Create Vertex Buffer
    // Infos to create a buffer (doesn't include assigning memory)
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;                               // Size of buffer (size of all vertices)
    bufferInfo.usage = bufferUsageFlags;                        // Multiple types of buffer possible, we want Vertex Buffer
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;         // Similar to Swap Chain images, can share vertex buffers

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, buffer);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Buffer !");
    }

    // Get buffer memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

    // Allocate memory to buffer
    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits,    // Index of memory type on Physical Device that has required bit flags
        bufferProperties);
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : CPU can interact with memory
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : Allows placement of data straight into buffer after mapping (otherwise we'd have to specify manually)

    // Allocate memory to vkDeviceMemory
    result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, memory);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate vertex buffer memory !");
    }

    // Allocate memory to given vertex buffer
    result = vkBindBufferMemory(device, *buffer, *memory, 0);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to bind buffer memory !");
    }
}