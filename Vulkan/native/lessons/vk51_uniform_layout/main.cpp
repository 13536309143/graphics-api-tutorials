#include "advanced_labs.hpp"
#include <exception>
#include <iostream>
// 常量字节布局：先读教材的参数约束，再运行独立实验核对实际设备和结果。
int main(int argc, char** argv) try {
    return course::nativeLabMain(argc, argv, 51);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
