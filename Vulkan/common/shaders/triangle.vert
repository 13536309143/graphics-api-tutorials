#version 450
layout(location=0) out vec3 normal;layout(location=1) out vec2 uv;layout(location=2) out vec3 worldPos;
void main(){vec2 points[3]=vec2[](vec2(-.7,.6),vec2(.7,.6),vec2(0,-.6));vec3 colors[3]=vec3[](vec3(1,0,0),vec3(0,1,0),vec3(0,0,1));normal=colors[gl_VertexIndex];uv=vec2(0);worldPos=vec3(points[gl_VertexIndex],0);gl_Position=vec4(worldPos,1);}
