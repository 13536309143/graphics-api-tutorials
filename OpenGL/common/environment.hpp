#pragma once
#include "advanced.hpp"
struct Environment {
    GLuint environment=0,irradiance=0,prefilter=0,brdf=0;
    static GLuint cubeTexture(int size,int levels){GLuint id;glGenTextures(1,&id);glBindTexture(GL_TEXTURE_CUBE_MAP,id);
        for(int m=0;m<levels;++m)for(int face=0;face<6;++face)glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+face,m,GL_RGB16F,std::max(1,size>>m),std::max(1,size>>m),0,GL_RGB,GL_FLOAT,nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,levels>1?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR);glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAX_LEVEL,levels-1);
        for(GLenum axis:{GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T,GL_TEXTURE_WRAP_R})glTexParameteri(GL_TEXTURE_CUBE_MAP,axis,GL_CLAMP_TO_EDGE);return id;}
    explicit Environment(const App& app,bool full=true){
        int w,h,c;float* pixels=stbi_loadf(asset(app,"studio.hdr").string().c_str(),&w,&h,&c,3);if(!pixels)throw std::runtime_error("HDR load failed");
        GLuint panorama,fbo;glGenTextures(1,&panorama);glBindTexture(GL_TEXTURE_2D,panorama);glTexImage2D(GL_TEXTURE_2D,0,GL_RGB16F,w,h,0,GL_RGB,GL_FLOAT,pixels);stbi_image_free(pixels);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glGenFramebuffers(1,&fbo);Mesh box(cube()),plane(quad(),quadIndices());glDisable(GL_DEPTH_TEST);
        const glm::vec3 directions[]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
        const glm::vec3 ups[]={{0,-1,0},{0,-1,0},{0,0,1},{0,0,-1},{0,-1,0},{0,-1,0}};
        auto capture=[&](const Shader& shader,GLuint target,int size,int levels){shader.use();shader.set("projection",glm::perspective(glm::radians(90.f),1.f,.1f,10.f));
            glBindFramebuffer(GL_FRAMEBUFFER,fbo);
            for(int m=0;m<levels;++m){shader.set("roughness",float(m)/std::max(levels-1,1));glViewport(0,0,size>>m,size>>m);
                for(int f=0;f<6;++f){shader.set("view",glm::lookAt(glm::vec3(0),directions[f],ups[f]));glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_CUBE_MAP_POSITIVE_X+f,target,m);RenderTarget::check();glClear(GL_COLOR_BUFFER_BIT);box.draw();}}};
        environment=cubeTexture(128,1);Shader convert(app,"capture.vert","equirect.frag");convert.use();convert.set("panorama",0);bind2D(panorama,0);capture(convert,environment,128,1);
        if(full){
            irradiance=cubeTexture(16,1);Shader diffuse(app,"capture.vert","irradiance.frag");diffuse.use();diffuse.set("environmentMap",0);bindCube(environment,0);capture(diffuse,irradiance,16,1);
            prefilter=cubeTexture(64,5);Shader specular(app,"capture.vert","prefilter.frag");specular.use();specular.set("environmentMap",0);bindCube(environment,0);capture(specular,prefilter,64,5);
            glGenTextures(1,&brdf);bind2D(brdf,0);glTexImage2D(GL_TEXTURE_2D,0,GL_RG16F,128,128,0,GL_RG,GL_FLOAT,nullptr);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,brdf,0);RenderTarget::check();glViewport(0,0,128,128);Shader integrate(app,"screen.vert","brdf.frag");integrate.use();plane.draw();
        }
        glDeleteTextures(1,&panorama);glDeleteFramebuffers(1,&fbo);defaultTarget(app);glEnable(GL_DEPTH_TEST);glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    }
    ~Environment(){glDeleteTextures(1,&environment);glDeleteTextures(1,&irradiance);glDeleteTextures(1,&prefilter);glDeleteTextures(1,&brdf);}
    Environment(const Environment&)=delete;Environment& operator=(const Environment&)=delete;
    void bindIBL()const{bindCube(irradiance,4);bindCube(prefilter,5);bind2D(brdf,6);}
    void sky(const Shader& shader,const Mesh& box,const App& app,const Camera& camera,bool encode=true)const{
        glDepthFunc(GL_LEQUAL);glDepthMask(GL_FALSE);shader.use();cameraUniforms(shader,app,camera);shader.set("displayEncode",encode);shader.set("environmentMap",0);bindCube(environment,0);box.draw();glDepthMask(GL_TRUE);glDepthFunc(GL_LESS);
    }
};
