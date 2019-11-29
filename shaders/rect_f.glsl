#version 400 core

in vec2 pass_coord;

out vec4 out_color;

uniform sampler2D u_backimg;
uniform sampler2D u_frontimg;

void main() {
    float d = 1.0 - texture(u_frontimg, pass_coord).r;
    float rx = 0.01 * fract(cos(1000.0 * (pass_coord.x + pass_coord.y + d + fract(cos(pass_coord.x / pass_coord.y + d) / 0.333))));
    float ry = 0.01 * fract(sin(1000.0 * (pass_coord.x + pass_coord.y + d + fract(sin(pass_coord.y / pass_coord.x + d) / 0.333))));
    float back = 1.0 - texture(u_backimg, pass_coord + vec2(rx, ry)).r;
    float front = 1.0 - texture(u_frontimg, pass_coord).r;
    if (texture(u_backimg, pass_coord).r == 1.0) discard;

    back *= 100.0;
    front *= 100.0;

    float depthdif = 1.0 / pow(4.0 * max(0.0, back - front), 0.5);
    depthdif = tanh(depthdif);
    out_color = vec4(vec3(0.0, 0.6, 1.0) * pow(depthdif, 2.0), 1.0);
}