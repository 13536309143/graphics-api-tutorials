#include "advanced.hpp"

// 线性颜色与 HDR，先阅读本目录 README.md。
int main(int argc,char** argv) try {
    App app(argc,argv,"21_hdr - OpenGL Course");
    RenderTarget scene(2),ping,pong;Shader shader(app,"advanced.vert","hdr_scene.frag"),tone(app,"screen.vert","tone.frag"),blur(app,"screen.vert","blur.frag");
    Mesh ball(sphere()),plane(quad(),quadIndices());Camera camera;camera.pos={0,0,8};camera.pitch=0;float exposure=1;
    while(app.begin()){
        if(app.width<1||app.height<1){app.end();continue;}
        camera.update(app);if(glfwGetKey(app.window,GLFW_KEY_UP)==GLFW_PRESS)exposure+=app.dt;if(glfwGetKey(app.window,GLFW_KEY_DOWN)==GLFW_PRESS)exposure=std::max(.05f,exposure-app.dt);
        scene.resize(app.width,app.height);ping.resize(app.width,app.height);pong.resize(app.width,app.height);scene.bind();glEnable(GL_DEPTH_TEST);app.clear();shader.use();cameraUniforms(shader,app,camera);
        for(int n=0;n<3;++n){auto model=glm::translate(glm::mat4(1),glm::vec3((n-1)*2.1f,0,0));shader.set("model",glm::scale(model,glm::vec3(.7f)));shader.set("baseColor",n==0?glm::vec3(.2f,.6f,1):n==1?glm::vec3(1,.35f,.08f):glm::vec3(.2f,1,.3f));shader.set("emission",n==1?5.f:0.f);ball.draw();}
        glDisable(GL_DEPTH_TEST);GLuint blurred=scene.colors[1];
        defaultTarget(app);app.clear();tone.use();tone.set("tex",0);tone.set("bloomTex",1);tone.set("exposure",exposure);tone.set("toneMap",app.key(GLFW_KEY_SPACE)!=GLFW_PRESS);tone.set("bloom",0);bind2D(scene.colors[0],0);bind2D(blurred,1);plane.draw();app.end();
    }
    return 0;
} catch(const std::exception& e){return failure(e);}
