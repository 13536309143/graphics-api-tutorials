#version 330 core
in vec3 direction;out vec4 FragColor;uniform sampler2D panorama;
void main(){vec3 d=normalize(direction);vec2 uv=vec2(atan(d.z,d.x)/6.2831853+0.5,acos(clamp(d.y,-1.0,1.0))/3.14159265);FragColor=vec4(texture(panorama,uv).rgb,1);}
