#version 330 core
in vec2 vTexCoord;
in vec4 vColor;
uniform sampler2D uTexture;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 outEmissive;

void main()
{
    outEmissive = vec4(0.0);
    float alpha = texture(uTexture, vTexCoord).r;
    FragColor = vec4(vColor.rgb, vColor.a * alpha);
}