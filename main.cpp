#include "VulkanRenderer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <string>

GLFWwindow * window;
VulkanRenderer vulkanRenderer;

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
    initWindow("First Vulkan Prototype");

    if (vulkanRenderer.init(window) == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }

    // Loop until closed.
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();


    }

    // Destroying the window.
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}