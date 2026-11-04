#pragma once
#include "advanced.hpp"
struct Multisample {
    GLuint fbo=0,color=0,depth=0;int width=0,height=0,samples=4;
    Multisample(){glGenFramebuffers(1,&fbo);glGenRenderbuffers(1,&color);glGenRenderbuffers(1,&depth);GLint maximum;glGetIntegerv(GL_MAX_SAMPLES,&maximum);samples=std::min(4,maximum);}
    ~Multisample(){glDeleteRenderbuffers(1,&color);glDeleteRenderbuffers(1,&depth);glDeleteFramebuffers(1,&fbo);}
    Multisample(const Multisample&)=delete;Multisample& operator=(const Multisample&)=delete;
    void resize(int w,int h){if(w==width && h==height)return;width=w;height=h;glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        glBindRenderbuffer(GL_RENDERBUFFER,color);glRenderbufferStorageMultisample(GL_RENDERBUFFER,samples,GL_RGBA8,w,h);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,color);
        glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorageMultisample(GL_RENDERBUFFER,samples,GL_DEPTH24_STENCIL8,w,h);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,depth);RenderTarget::check();}
};
