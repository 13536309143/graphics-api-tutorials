#version 330 core
in vec2 uv;out vec4 FragColor;uniform sampler2D tex;
void main(){vec3 c=texture(tex,uv).rgb;FragColor=vec4(dot(c,vec3(.2126,.7152,.0722))>1.0?c:vec3(0),1);}
