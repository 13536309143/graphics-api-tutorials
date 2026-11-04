#include "advanced.hpp"

// 视差遮蔽映射，先阅读本目录 README.md。
int main(int argc,char** argv) try {
    App app(argc,argv,"20_parallax - OpenGL Course");
    Shader shader(app,"advanced.vert","surface.frag");Mesh plane(quad(),quadIndices());Camera camera;camera.pos={0,0,3};camera.pitch=0;
    ImageTexture albedo(asset(app,"textures/albedo.png"),true),normal(asset(app,"textures/normal.png")),height(asset(app,"textures/height.png"));
    glEnable(GL_DEPTH_TEST);
    while(app.begin()){
        camera.update(app);app.clear();shader.use();surfaceDefaults(shader);cameraUniforms(shader,app,camera);
        albedo.bind(0);normal.bind(1);height.bind(2);shader.set("useTexture",1);shader.set("baseColor",glm::vec3(1));
        shader.set("useNormal",app.key(GLFW_KEY_SPACE)!=GLFW_PRESS);
        shader.set("useParallax",app.key(GLFW_KEY_SPACE)!=GLFW_PRESS);
        shader.set("model",glm::scale(glm::mat4(1),glm::vec3(2)));plane.draw();app.end();
    }
    return 0;
} catch(const std::exception& e){return failure(e);}
