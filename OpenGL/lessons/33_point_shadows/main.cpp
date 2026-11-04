#include "advanced.hpp"

// 点光源全方向阴影，先阅读本目录 README.md。
int main(int argc,char** argv) try {
    App app(argc,argv,"33_point_shadows - OpenGL Course");
    GLuint fbo,depthCube;glGenFramebuffers(1,&fbo);glGenTextures(1,&depthCube);bindCube(depthCube,0);
    for(int f=0;f<6;++f)glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+f,0,GL_DEPTH_COMPONENT24,512,512,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_NEAREST);for(GLenum axis:{GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T,GL_TEXTURE_WRAP_R})glTexParameteri(GL_TEXTURE_CUBE_MAP,axis,GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER,fbo);glDrawBuffer(GL_NONE);glReadBuffer(GL_NONE);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_CUBE_MAP_POSITIVE_X,depthCube,0);RenderTarget::check();
    Shader depth(app,"point_depth.vert","point_depth.frag"),shader(app,"advanced.vert","point_shadow.frag");Mesh box(cube());Camera camera;camera.pos={0,3.5f,8};camera.pitch=-20;glEnable(GL_DEPTH_TEST);
    const glm::vec3 directions[]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}},ups[]={{0,-1,0},{0,-1,0},{0,0,1},{0,0,-1},{0,-1,0},{0,-1,0}};
    auto draw=[&](const Shader& s){s.set("model",glm::scale(glm::translate(glm::mat4(1),glm::vec3(0,-1,0)),glm::vec3(14,.2f,14)));box.draw();for(int k=0;k<4;++k){auto m=glm::translate(glm::mat4(1),glm::vec3(std::cos(k*1.5708f)*2,0,std::sin(k*1.5708f)*2));s.set("model",m);box.draw();}};
    while(app.begin()){
        camera.update(app);glm::vec3 light(std::sin(app.time())*.7f,2,std::cos(app.time())*.7f);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glViewport(0,0,512,512);depth.use();depth.set("light",light);depth.set("farPlane",25.f);
        for(int f=0;f<6;++f){glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_CUBE_MAP_POSITIVE_X+f,depthCube,0);glClear(GL_DEPTH_BUFFER_BIT);depth.set("lightVP",glm::perspective(glm::radians(90.f),1.f,.1f,25.f)*glm::lookAt(light,light+directions[f],ups[f]));draw(depth);}
        defaultTarget(app);app.clear();shader.use();cameraUniforms(shader,app,camera);shader.set("light",light);shader.set("farPlane",25.f);shader.set("baseColor",glm::vec3(.3f,.65f,.8f));shader.set("depthCube",0);shader.set("shadows",app.key(GLFW_KEY_SPACE)!=GLFW_PRESS);bindCube(depthCube,0);draw(shader);app.end();
    }glDeleteTextures(1,&depthCube);glDeleteFramebuffers(1,&fbo);
    return 0;
} catch(const std::exception& e){return failure(e);}
