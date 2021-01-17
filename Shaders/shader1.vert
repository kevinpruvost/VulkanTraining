#version 450        // Use GLGL 4.50

layout(location = 0) in vec3 pos;    // Ouput color for vertex (location is required)
layout(location = 1) in vec3 col;

layout(binding = 0) uniform MVP {
    mat4 projection;
    mat4 view;
    mat4 model;
} mvp;

layout(location = 0) out vec3 fragCol;

layout(location = 0)

void main() {
    gl_Position = mvp.projection * mvp.view * mvp.model * vec4(pos, 1.0);

    fragCol = col;
}