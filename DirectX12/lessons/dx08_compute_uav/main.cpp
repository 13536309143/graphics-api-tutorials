#include "dx12_course.hpp"
int main(int argc, char** argv) try { return course::dxMain(argc, argv, 8); } catch (const std::exception& e) {
  std::cerr << e.what() << "\n";
  return 1;
}
