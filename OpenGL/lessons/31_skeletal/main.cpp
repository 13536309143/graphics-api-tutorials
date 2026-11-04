#include "model.hpp"

// 骨骼动画与蒙皮，先阅读本目录 README.md。
int main(int argc,char** argv) try {
    App app(argc,argv,"31_skeletal - OpenGL Course");
    auto modelPath=asset(app,"models/bending.gltf");
    for(int k=1;k+1<argc;++k)if(std::string(argv[k])=="--model")modelPath=std::filesystem::absolute(argv[++k]);
    Model model(modelPath);
    Shader shader(app,"model.vert","pbr.frag"); Camera camera;
    camera.pos=glm::vec3(0,1.5f,6);camera.pitch=-6;camera.yaw=-90;
    glEnable(GL_DEPTH_TEST);
    while(app.begin()){
        camera.update(app);app.clear();shader.use();pbrDefaults(shader);cameraUniforms(shader,app,camera);
        // 空格按下时暂停在 bind pose，便于对照骨骼形变。
        model.update(app.key(GLFW_KEY_SPACE)==GLFW_PRESS?0.f:app.time());
        model.draw(shader);app.end();
    }
    return 0;
} catch(const std::exception& e){return failure(e);}
