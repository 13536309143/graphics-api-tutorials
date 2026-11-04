#include "environment.hpp"

// IBL 与 Split Sum，先阅读本目录 README.md。
int main(int argc,char** argv) try {
    App app(argc,argv,"25_ibl - OpenGL Course");
    Environment env(app);Shader shader(app,"advanced.vert","pbr.frag"),sky(app,"sky.vert","sky.frag");Mesh ball(sphere()),box(cube());Camera camera;camera.pos={0,0,14};camera.pitch=0;
    while(app.begin()){
        camera.update(app);app.clear();shader.use();pbrDefaults(shader);cameraUniforms(shader,app,camera);env.bindIBL();shader.set("useIBL",app.key(GLFW_KEY_SPACE)!=GLFW_PRESS);
        for(int y=0;y<5;++y)for(int x=0;x<7;++x){shader.set("metallic",float(y)/4);shader.set("roughness",std::max(.05f,float(x)/6));auto model=glm::translate(glm::mat4(1),glm::vec3((x-3)*1.65f,(y-2)*1.65f,0));shader.set("model",glm::scale(model,glm::vec3(.65f)));ball.draw();}
        env.sky(sky,box,app,camera);app.end();
    }
    return 0;
} catch(const std::exception& e){return failure(e);}
