#include "Mesh.hpp"

Mesh::Mesh()
{
}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<Vertex>* vertices)
{
    vertexCount = vertices->size();
    physicalDevice = newPhysicalDevice;
    device = newDevice;
    createVertexBuffer(vertices);
}

Mesh::~Mesh()
{
}

void Mesh::destroyVertexBuffer()
{
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
}

int Mesh::getVertexCount()
{
    return vertexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
    return vertexBuffer;
}

void Mesh::createVertexBuffer(std::vector<Vertex>* vertices)
{
    // Get size of buffer needed for vertices
    VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

    // Create buffer and allocate memory to it
    createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &vertexBuffer, &vertexBufferMemory);

    // Map Memory to Vertex Buffer
    void * data;                                                            // 1. Create a pointer to a point in normal memory
    vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data);  // 2. Map the vertex buffer memory to that point
    memcpy(data, vertices->data(), static_cast<size_t>(bufferSize));   // 3. Copy memory from vertices to the point
    vkUnmapMemory(device, vertexBufferMemory);                              // 4. Unmap the vertex buffer memory
}