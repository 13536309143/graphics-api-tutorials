#include "app.hpp"

// Shader 与 Uniform 动画：配合本目录 README.md 阅读。
int main(int argc,char** argv) try {
    App app(argc,argv,"04_uniform - OpenGL Course");
    Shader shader(app,"basic.vert","color.frag");
    Mesh mesh(quad(),quadIndices());
    while(app.begin()){
        app.clear(); shader.use();
        glm::mat4 mvp(1.f);
        shader.set("mvp",mvp);
        shader.set("pulse",0.55f+0.45f*std::sin(app.time()*2));
        mesh.draw(); app.end();
    }
    return 0;
} catch(const std::exception& e){return failure(e);}
