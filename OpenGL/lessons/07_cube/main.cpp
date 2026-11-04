#include "app.hpp"

// 三维坐标与深度测试：配合本目录 README.md 阅读。
int main(int argc,char** argv) try {
    App app(argc,argv,"07_cube - OpenGL Course");
    Shader shader(app,"basic.vert","texture.frag");
    Mesh mesh(cube()); Texture texture; Camera camera;
    glEnable(GL_DEPTH_TEST);
    while(app.begin()){
        app.clear(); shader.use(); texture.bind(); shader.set("tex",0);
        auto model=glm::rotate(glm::mat4(1),app.time()*.7f,glm::vec3(.5f,1,.2f));
        auto view=glm::lookAt(glm::vec3(2,1.5f,3),glm::vec3(0),glm::vec3(0,1,0));
        auto proj=glm::perspective(glm::radians(45.f),app.aspect(),.1f,100.f);
        shader.set("mvp",proj*view*model); mesh.draw();
        app.end();
    }
    return 0;
} catch(const std::exception& e){return failure(e);}
