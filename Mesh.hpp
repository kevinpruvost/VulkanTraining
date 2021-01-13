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
         VkCommandPool transferCommandPool, std::vector<Vertex> * vertices, std::vector<uint32_t> * indices);

    ~Mesh();
    void destroyVertexBuffer();

    int getVertexCount();
    VkBuffer getVertexBuffer();

    int getIndexCount();
    VkBuffer getIndexBuffer();

private:
    void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool,
                            std::vector<Vertex> * vertices);
    void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool,
        std::vector<uint32_t>* indices);

private:
    int vertexCount;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    int indexCount;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkPhysicalDevice physicalDevice;
    VkDevice device;
};