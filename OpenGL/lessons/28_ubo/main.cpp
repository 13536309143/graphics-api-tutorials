#include "advanced.hpp"

// UBO 与 std140，先阅读本目录 README.md。
int main(int argc,char** argv) try {
    App app(argc,argv,"28_ubo - OpenGL Course");
    Shader shader(app,"ubo.vert","solid.frag");Mesh box(cube());Camera camera;glEnable(GL_DEPTH_TEST);
    GLuint buffer;glGenBuffers(1,&buffer);glBindBuffer(GL_UNIFORM_BUFFER,buffer);glBufferData(GL_UNIFORM_BUFFER,2*sizeof(glm::mat4),nullptr,GL_DYNAMIC_DRAW);
    GLuint block=glGetUniformBlockIndex(shader.id,"CameraBlock");if(block==GL_INVALID_INDEX)throw std::runtime_error("Missing uniform block");glUniformBlockBinding(shader.id,block,0);glBindBufferBase(GL_UNIFORM_BUFFER,0,buffer);
    while(app.begin()){
        camera.update(app);app.clear();auto projection=camera.projection(app),view=camera.view();
        glBindBuffer(GL_UNIFORM_BUFFER,buffer);glBufferSubData(GL_UNIFORM_BUFFER,0,sizeof(glm::mat4),glm::value_ptr(projection));glBufferSubData(GL_UNIFORM_BUFFER,sizeof(glm::mat4),sizeof(glm::mat4),glm::value_ptr(view));
        shader.use();shader.set("opacity",1.f);
        for(int n=0;n<5;++n){auto model=glm::translate(glm::mat4(1),glm::vec3((n-2)*1.3f,0,0));shader.set("model",glm::rotate(model,app.time()*.3f+n,glm::vec3(.2f,1,.1f)));shader.set("baseColor",glm::vec3(.15f*n,.6f,1-.15f*n));box.draw();}app.end();
    }glDeleteBuffers(1,&buffer);
    return 0;
} catch(const std::exception& e){return failure(e);}
