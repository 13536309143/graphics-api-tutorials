#version 330 core
in vec3 color;
out vec4 FragColor;
uniform float pulse;
void main(){ FragColor=vec4(color*pulse,1.0); }
