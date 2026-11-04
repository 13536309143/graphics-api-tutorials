#include "app.hpp"

// 窗口与渲染循环：配合本目录 README.md 阅读。
int main(int argc,char** argv) try {
    App app(argc,argv,"01_window - OpenGL Course");
    // 渲染循环 = 处理事件 → 设置视口 → 清屏 → 交换前后缓冲。
    while(app.begin()){app.clear(); app.end();}
    return 0;
} catch(const std::exception& e){return failure(e);}
