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