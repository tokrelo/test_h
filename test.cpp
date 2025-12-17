#include "test.h"

void does_throw() {
  throw std::runtime_error("oh oh");
}

void does_not_throw() {

}

int main() {
  std::cout << "These checks should all be true: " << std::endl;
  check(true);
  check(1, 1);
  check(1., 1, "one is one");
  check(std::string("abc"), "abc");
  check_exception(does_throw());

  std::cout << "\n\nThese checks should all be false" << std::endl;
  check(false);
  check("abc", "cde");
  check(1.5, 1);
  check(1, 2, "Error message");
  check_exception(does_not_throw());
}