#version 330 core
out vec4 FragColor;
in vec2 vUV;

uniform sampler2D uImage;
uniform bool uHorizontal;

void main()
{
    vec2 texelSize = 1.0 / textureSize(uImage, 0);
    vec3 result = texture(uImage, vUV).rgb * 0.227027;

    vec2 offset = uHorizontal ? vec2(texelSize.x, 0.0)
                              : vec2(0.0, texelSize.y);

    result += texture(uImage, vUV + offset * 1.384615).rgb * 0.316216;
    result += texture(uImage, vUV - offset * 1.384615).rgb * 0.316216;
    result += texture(uImage, vUV + offset * 3.230769).rgb * 0.070270;
    result += texture(uImage, vUV - offset * 3.230769).rgb * 0.070270;

    FragColor = vec4(result, 1.0);
}