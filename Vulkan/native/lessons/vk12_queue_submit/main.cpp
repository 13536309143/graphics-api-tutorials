#include "advanced_labs.hpp"
#include <exception>
#include <iostream>

// 提交与完成条件：实验实现集中于 common/advanced_labs.cpp，避免复制资源所有权代码。
int main(int argc, char** argv) try {
    return course::nativeLabMain(argc, argv, 12);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
