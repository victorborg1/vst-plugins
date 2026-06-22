
#version 330 core

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outEmissive;

in vec2 vPos;
in vec4 vColor;

uniform vec2  uCenter;
uniform float uRadius;
uniform float uThickness;
uniform float uStartAngle;
uniform float uEndAngle;
uniform bool  uFilled;
uniform bool  uClockwise;
uniform bool  uFullCircle;
uniform float uEmissiveStrength; 

uniform vec4  uColorTop;
uniform vec4  uColorBottom;

bool angleInArc(float a, float start, float end, bool cw, bool full)
{
    if (full) return true;
    if (!cw)
        return start <= end ? (a >= start && a <= end) : (a >= start || a <= end);
    else
        return end <= start ? (a <= start && a >= end) : (a <= start || a >= end);
}

void main()
{
    vec2 dir = vPos - uCenter;
    float dist = length(dir);

    float angle = atan(-dir.y, dir.x);
    if (angle < 0.0) angle += 6.2831853;

    float alpha = 0.0;

    if (angleInArc(angle, uStartAngle, uEndAngle, uClockwise, uFullCircle))
    {
        if (uFilled)
        {
            alpha = 1.0 - smoothstep(uRadius - 1.0, uRadius + 1.0, dist);
        }
        else
        {
            float innerR = uRadius - uThickness * 0.5;
            float outerR = uRadius + uThickness * 0.5;
            alpha = smoothstep(innerR - 1.0, innerR + 1.0, dist)
                  * (1.0 - smoothstep(outerR - 1.0, outerR + 1.0, dist));
        }
    }

    float t = (vPos.y - (uCenter.y - uRadius)) / (uRadius * 2.0);
    t = clamp(t, 0.0, 1.0);
    vec4 color = mix(uColorTop, uColorBottom, t);

    outColor    = vec4(color.rgb, color.a * alpha);
    outEmissive = vec4(color.rgb * uEmissiveStrength * alpha, color.a * alpha);
}