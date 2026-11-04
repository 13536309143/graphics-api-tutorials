#version 330 core
in vec2 uv;in float fade;out vec4 FragColor;
void main(){float r=length(uv-.5)*2.0;if(r>=1.0)discard;float alpha=(1.0-r)*(1.0-fade);FragColor=vec4(mix(vec3(1,.7,.08),vec3(.9,.08,.01),fade),alpha);}
