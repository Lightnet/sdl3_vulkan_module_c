#version 450
layout(location = 0) in vec3 inPosition; // Matches the vertex input in the C code
layout(location = 1) in vec3 inColor;    // Matches the vertex input in the C code
layout(location = 0) out vec3 fragColor; // Passes color to fragment shader

void main() {
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0); // Outputs a default position (center of screen)
    fragColor = vec3(0.0, 0.0, 0.0);        // Outputs black color
}