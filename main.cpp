#define VULKAN_DEBUG
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

    float angle = 0.0f;
    float deltaTime = 0.0f;
    float lastTime = 0.0f;

    // Loop until closed.
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float now = glfwGetTime();
        deltaTime = now - lastTime;
        lastTime = now;

        angle += 10.0f * deltaTime;

        if (angle > 360.0f) angle -= 360.0f;

        glm::mat4 firstModel(1.0f);
        glm::mat4 secondModel(1.0f);

        firstModel = glm::translate(firstModel, glm::vec3(-2.0f, 0.0f, -5.0f));
        firstModel = glm::rotate(firstModel, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));

        secondModel = glm::translate(secondModel, glm::vec3(2.0f, 0.0f, -5.0f));
        secondModel = glm::rotate(secondModel, glm::radians(-angle * 10), glm::vec3(0.0f, 0.0f, 1.0f));

        vulkanRenderer.updateModel(0, firstModel);
        vulkanRenderer.updateModel(1, secondModel);

        vulkanRenderer.draw();
    }

    vulkanRenderer.destroy();

    // Destroying the window.
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}