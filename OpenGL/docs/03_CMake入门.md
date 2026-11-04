# CMake 入门

CMake 负责描述和生成构建系统，MSVC 才是编译器。configure 阶段检测工具链并生成工程；build 阶段才编译与链接。CMakeLists.txt 是项目本体，CMakePresets.json 保存常用配置，build/ 中的缓存与项目文件都属于生成物。

阅读根 CMakeLists.txt：project 启用 C 和 CXX；CMAKE_CXX_STANDARD=17 选择语言标准；add_subdirectory 引入本地 GLFW；add_library(glad STATIC ...) 编译 GLAD；course 是 INTERFACE target，把 include 路径、编译选项、依赖关系传播给各课。

`target_link_libraries(02_triangle PRIVATE course)` 除了链接库，也会获取 course 的头文件路径和选项。这样不需要全局 include_directories 或手工填写某个电脑的 .lib 路径。GLFW 与 GLAD 静态链接进示例，不需要另外复制 glfw3.dll。

file(GLOB ... CONFIGURE_DEPENDS) 扫描 lessons 子目录，为每个 main.cpp 创建一个独立程序。新课可复制一个目录并改名，再配置构建。大型产品通常显式列出源码；这里自动发现便于练习。不要把所有课的 main.cpp 链接到同一程序，否则会发生 main 重复定义。

POST_BUILD 将 shaders 复制到程序旁。只修改 GLSL 时，也执行一次构建，让复制步骤更新运行目录；不要只修改 build/bin 下的文件，它们下次会被源码覆盖。

练习：复制 02_triangle 为 14_exercise，重新执行 configure/build；用 `--target 14_exercise` 只编译你的程序。给 course 添加编译定义后解释为什么所有课都能收到。
