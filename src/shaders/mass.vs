#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform mat4 uVP;
out vec3 vColor;
void main() {
    gl_Position = uVP * vec4(aPos, 1.0);
    vColor = aColor;
    gl_PointSize = 10.0;
}