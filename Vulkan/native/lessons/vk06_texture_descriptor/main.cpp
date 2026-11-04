#include "native_app.hpp"
int main(int argc, char** argv) try { return course::nativeMain(argc, argv, 6); } catch (const std::exception& e) {
  std::cerr << e.what() << "\n";
  return 1;
}
