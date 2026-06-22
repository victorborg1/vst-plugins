#version 330 core
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outEmissive;

in vec2 vPos;
in vec4 vColor;

uniform vec2  uP1;
uniform vec2  uP2;
uniform float uThickness;
uniform float uEmissiveStrength;


float sdCapsule(vec2 p, vec2 a, vec2 b, float r)
{
    vec2 ab = b - a;
    vec2 ap = p - a;
    float t = clamp(dot(ap, ab) / dot(ab, ab), 0.0, 1.0);
    vec2 closest = a + t * ab;
    return length(p - closest) - r;
}

void main()
{
    float r    = uThickness * 0.5;
    float dist = sdCapsule(vPos, uP1, uP2, r);

    float alpha = 1.0 - smoothstep(-1.0, 1.0, dist);

    outColor    = vec4(vColor.rgb, vColor.a * alpha);
    outEmissive = vec4(vColor.rgb * uEmissiveStrength * alpha, vColor.a * alpha);
}