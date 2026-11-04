#include "advanced_labs.hpp"
#include <exception>
#include <iostream>
// 窗口呈现能力：先读教材的参数约束，再运行独立实验核对实际设备和结果。
int main(int argc, char** argv) try {
    return course::nativeLabMain(argc, argv, 40);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
