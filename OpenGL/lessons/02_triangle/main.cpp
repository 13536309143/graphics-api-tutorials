#include "app.hpp"

// 第一个彩色三角形：配合本目录 README.md 阅读。
int main(int argc,char** argv) try {
    App app(argc,argv,"02_triangle - OpenGL Course");
    Shader shader(app,"basic.vert","color.frag");
    Mesh mesh({-.7f,-.5f,0, 1,0,0, 0,0, .7f,-.5f,0, 0,1,0, 1,0, 0,.6f,0, 0,0,1, .5f,1});
    while(app.begin()){
        app.clear(); shader.use();
        glm::mat4 mvp(1.f);
        shader.set("mvp",mvp);
        shader.set("pulse",1.f);
        mesh.draw(); app.end();
    }
    return 0;
} catch(const std::exception& e){return failure(e);}
