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
    Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue,
         VkCommandPool transferCommandPool, std::vector<Vertex> * vertices);

    ~Mesh();
    void destroyVertexBuffer();

    int getVertexCount();
    VkBuffer getVertexBuffer();

private:
    void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool,
                            std::vector<Vertex> * vertices);

private:
    int vertexCount;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkPhysicalDevice physicalDevice;
    VkDevice device;
};