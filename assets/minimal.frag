#version 450
layout(location = 0) in vec3 fragColor;  // Receives color from vertex shader
layout(location = 0) out vec4 outColor;  // Outputs to the framebuffer

void main() {
    outColor = vec4(fragColor, 1.0); // Outputs the input color with full opacity
}