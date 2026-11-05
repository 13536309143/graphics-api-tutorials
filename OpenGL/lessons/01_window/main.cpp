// 01_window
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/app.hpp"

// 窗口与渲染循环：配合本目录 README.md 阅读。
// 本课重点：创建 3.3 Core 上下文，观察事件循环、清屏与缓冲交换。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "01_window - OpenGL Course");
    // 渲染循环 = 处理事件 → 设置视口 → 清屏 → 交换前后缓冲。
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        app.clear();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
