#include "VulkanRenderer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <string>

GLFWwindow * window;

void initWindow(const std::string & wName = "Tuto",
    const int width = 800, const int height = 600)
{
    // Init GLFW.
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Important pour VULKAN pour ne pas OPENGL
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}

int main()
{
    initWindow();


    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    printf("Extension count : %i\n", extensionCount);

    glm::mat4 testMatrix(1.0f);
    glm::mat4 testVector(1.0f);

    auto testResult = testMatrix * testVector;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();


    }

    glfwDestroyWindow(window);

    return EXIT_SUCCESS;
}