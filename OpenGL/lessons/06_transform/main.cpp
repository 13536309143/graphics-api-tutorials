#include "app.hpp"

// 二维变换：配合本目录 README.md 阅读。
int main(int argc,char** argv) try {
    App app(argc,argv,"06_transform - OpenGL Course");
    Shader shader(app,"basic.vert","texture.frag");
    Mesh mesh(quad(),quadIndices());
    Texture texture;
    while(app.begin()){
        app.clear(); shader.use();
        glm::mat4 mvp(1.f);
        mvp=glm::translate(mvp,glm::vec3(.25f,0,0));
        mvp=glm::rotate(mvp,app.time(),glm::vec3(0,0,1));
        mvp=glm::scale(mvp,glm::vec3(.6f));
        shader.set("mvp",mvp);
        texture.bind(); shader.set("tex",0);
        mesh.draw(); app.end();
    }
    return 0;
} catch(const std::exception& e){return failure(e);}
