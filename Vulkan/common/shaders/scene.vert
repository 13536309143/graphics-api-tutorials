#version 450
layout(location=0) in vec3 aPos;layout(location=1) in vec3 aNormal;layout(location=2) in vec2 aUV;
layout(binding=0) uniform Frame {mat4 viewProjection;} frame;
layout(push_constant) uniform Draw {mat4 model;vec4 settings;} draw;
layout(location=0) out vec3 normal;layout(location=1) out vec2 uv;layout(location=2) out vec3 worldPos;
void main(){int mode=int(draw.settings.x);vec3 p=aPos,n=aNormal;uv=aUV;
  if(mode==3){vec2 points[3]=vec2[](vec2(-.7,.6),vec2(.7,.6),vec2(0,-.6));p=vec3(points[gl_VertexIndex],0);n=vec3(gl_VertexIndex==0,gl_VertexIndex==1,gl_VertexIndex==2);uv=vec2(0);}
  if(mode==7)p+=vec3(float(gl_InstanceIndex%5-2)*1.5,0,-float(gl_InstanceIndex/5)*1.5);
  vec4 world=draw.model*vec4(p,1);worldPos=world.xyz;normal=mat3(transpose(inverse(draw.model)))*n;gl_Position=frame.viewProjection*world;
}
