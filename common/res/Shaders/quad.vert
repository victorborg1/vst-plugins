#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
uniform mat4 uProjection;
uniform mat4 uModel;
out vec4 vColor;
out vec2 vTexCoord;
void main() {
    vColor = aColor;
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
}