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
    // Create Vertex Buffer
    // Infos to create a buffer (doesn't include assigning memory)
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(Vertex) * vertices->size();        // Size of buffer (size of all vertices)
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;       // Multiple types of buffer possible, we want Vertex Buffer
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;         // Similar to Swap Chain images, can share vertex buffers

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Buffer !");
    }

    // Get buffer memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

    // Allocate memory to buffer
    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits,    // Index of memory type on Physical Device that has required bit flags
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : CPU can interact with memory
            // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : Allows placement of data straight into buffer after mapping (otherwise we'd have to specify manually)

    // Allocate memory to vkDeviceMemory
    result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &vertexBufferMemory);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate vertex buffer memory !");
    }

    // Allocate memory to given vertex buffer
    result = vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to bind buffer memory !");
    }

    // Map Memory to Vertex Buffer
    void * data;                                                            // 1. Create a pointer to a point in normal memory
    vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);  // 2. Map the vertex buffer memory to that point
    memcpy(data, vertices->data(), static_cast<size_t>(bufferInfo.size));   // 3. Copy memory from vertices to the point
    vkUnmapMemory(device, vertexBufferMemory);                              // 4. Unmap the vertex buffer memory
}

uint32_t Mesh::findMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties)
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
