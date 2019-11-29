#version 400 core

layout(location=0) in vec3 in_pos;
layout(location=1) in vec3 in_norm;

out vec3 pass_pos;
out vec3 pass_norm;

uniform mat4 u_mvp;

void main() {
    vec4 pos = u_mvp * vec4(in_pos, 1.0);
    pass_pos = pos.xyz / pos.w;
    pass_norm = inverse(transpose(mat3(u_mvp))) * in_norm;
    gl_Position = vec4(pos.xyz / pos.w, 1.0);
}
