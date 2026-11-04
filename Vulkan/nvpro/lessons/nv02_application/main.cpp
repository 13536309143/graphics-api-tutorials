#include "nvapp_course.hpp"
int main(int argc, char** argv) try { return course::nvproMain(argc, argv, 2); } catch (const std::exception& e) {
  std::cerr << e.what() << "\n";
  return 1;
}
