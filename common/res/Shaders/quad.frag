#version 330 core
in vec4 vColor;
in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform bool      uUseTexture;
uniform float     uEmissiveStrength;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outEmissive;

void main() {
    vec4 color;
    if (uUseTexture) {
        color = texture(uTexture, vTexCoord) * vColor;
    } else {
        color = vColor;
    }

    outColor    = color;
    outEmissive = vec4(color.rgb * uEmissiveStrength * color.a, color.a);
}