#version 450
layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D fontTexture;

void main() {
    float alpha = texture(fontTexture, fragTexCoord).r;
    outColor = vec4(1.0, 1.0, 1.0, alpha); // White text with alpha from texture
}