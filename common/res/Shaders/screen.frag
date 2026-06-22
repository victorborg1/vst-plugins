#version 330 core
out vec4 FragColor;
in vec2 vUV;

uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform float uBloomStrength;

void main()
{
    vec3 scene = texture(uScene, vUV).rgb;
    vec3 bloom = texture(uBloom, vUV).rgb;

    FragColor = vec4(scene + bloom * uBloomStrength, 1.0);
}