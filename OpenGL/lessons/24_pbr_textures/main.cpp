#include "advanced.hpp"

// PBR 贴图工作流，先阅读本目录 README.md。
int main(int argc,char** argv) try {
    App app(argc,argv,"24_pbr_textures - OpenGL Course");
    Shader shader(app,"advanced.vert","pbr.frag");Mesh ball(sphere());Camera camera;camera.pos={0,0,14};camera.pitch=0;
    ImageTexture albedo(asset(app,"textures/albedo.png"),true),normal(asset(app,"textures/normal.png")),mr(asset(app,"textures/metalrough.png")),ao(asset(app,"textures/ao.png"));
    glEnable(GL_DEPTH_TEST);
    while(app.begin()){
        camera.update(app);app.clear();shader.use();pbrDefaults(shader);cameraUniforms(shader,app,camera);
        albedo.bind(0);normal.bind(1);mr.bind(2);ao.bind(3);
        shader.set("useMaps",1);shader.set("useNormal",app.key(GLFW_KEY_SPACE)!=GLFW_PRESS);
        for(int y=0;y<5;++y)for(int x=0;x<7;++x){
            shader.set("metallic",float(y)/4);shader.set("roughness",std::max(.05f,float(x)/6));
            auto model=glm::translate(glm::mat4(1),glm::vec3((x-3)*1.65f,(y-2)*1.65f,0));
            shader.set("model",glm::scale(model,glm::vec3(.65f)));ball.draw();
        }app.end();
    }
    return 0;
} catch(const std::exception& e){return failure(e);}
