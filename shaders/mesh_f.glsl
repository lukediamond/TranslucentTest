#version 400 core

in vec3 pass_pos;
in vec3 pass_norm;

out vec4 out_color;

void main() {
    out_color.rgb = pass_norm * 0.5 + 0.5; 
    out_color.a = 1.0;
}
