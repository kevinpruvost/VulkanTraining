#version 450

// .frag 'in' can only connect to .vert 'out', not .frag 'in' with .frag 'out'

layout(location = 0) in vec3 fragCol;     // Final output color (must also have location)

layout(location = 0) out vec4 outColor;     // Final output color (must also have location)

void main() {
    outColor = vec4(fragCol, 1.0);
}