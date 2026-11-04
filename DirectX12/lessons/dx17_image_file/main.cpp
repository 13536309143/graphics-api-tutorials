#include "advanced_labs.hpp"
#include <exception>
#include <iostream>

// 真实图片上传：实验实现集中于 common/advanced_labs.cpp，避免复制资源所有权代码。
int main(int argc, char** argv) try {
    return course::dxLabMain(argc, argv, 17);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
