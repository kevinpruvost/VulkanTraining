#pragma once

// Project includes
#include "Utilities.hpp"

// GLFW includes
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ includes
#include <vector>

class Mesh
{
public:
    Mesh();
    Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<Vertex> * vertices);

    ~Mesh();
    void destroyVertexBuffer();

    int getVertexCount();
    VkBuffer getVertexBuffer();

private:
    void createVertexBuffer(std::vector<Vertex> * vertices);
    uint32_t findMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties);

private:
    int vertexCount;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkPhysicalDevice physicalDevice;
    VkDevice device;
};