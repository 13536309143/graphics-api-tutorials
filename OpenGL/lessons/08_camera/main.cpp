#include "app.hpp"

// 自由相机：配合本目录 README.md 阅读。
int main(int argc,char** argv) try {
    App app(argc,argv,"08_camera - OpenGL Course");
    Shader shader(app,"basic.vert","texture.frag");
    Mesh mesh(cube()); Texture texture; Camera camera;
    glEnable(GL_DEPTH_TEST);
    while(app.begin()){
        app.clear(); shader.use(); texture.bind(); shader.set("tex",0);
        camera.update(app);
        auto vp=camera.projection(app)*camera.view();
        for(int n=0;n<10;++n){
            auto model=glm::translate(glm::mat4(1),glm::vec3((n%5-2)*1.6f,0,-(n/5)*2.f));
            model=glm::rotate(model,app.time()*.3f+n,glm::vec3(1,.5f,.2f));
            shader.set("mvp",vp*model); mesh.draw();
        }
        app.end();
    }
    return 0;
} catch(const std::exception& e){return failure(e);}
