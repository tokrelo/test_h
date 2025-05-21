/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** test.h, an extremly simple test framework.
 * Version 1.7
 * Copyright (C) 2022-2024 Tobias Kreilos, Offenburg University of Applied
 * Sciences
 */

/**
 * The framework defines a function check(a,b) that can be called with
 * parameters of different types. The function asserts
 * that the two paramters are equal (within a certain, predefined range for
 * floating point numbers) and prints the result of the comparison on the
 * command line. Additionally a summary of all tests is printed at the end of
 * the program.
 * There is a TEST macro, which you can place outside main to group
 * tests together. Code in the macro is automatically executed at the beginning
 * of the program.
 * The file also defines a class InstanceCount, that can be used to
 * count how many instances of an object are still alive at the end of a
 * program. To use it, derive your class from InstanceCount<ClassName> and the
 * message is automatically printed at the end of the program.
 *
 * The functions are thread- and reentrant-safe. Support for OpenMP is included.
 * Execution with MPI is supported, but no collection of the results occurs. All
 * tests are executed locally, results are printed for every node separately.
 *
 * Caution: the TEST macro uses static storage of objects, so be aware of the
 * static initialization order fiasco when using multiple source files.
 *
 * Example usage:
 *
 * #include "test.h"
 * TEST(MyTest) {
 *   check(1, 1);
 * }
 *
 * int main() {
 *   const std::string s = "Hi";
 *   check(s, "Hi");
 * }
 */

#ifndef VERY_SIMPLE_TEST_H
#define VERY_SIMPLE_TEST_H

#include <atomic>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef _OPENMP
#include <omp.h>
#endif

/** Simple macro to execute the code that follows the macro (without call from
 * main)
 *
 * Define a class, that is directly instantiated
 * and contains the test code in the constructor.
 *
 * Usage:
 * TEST(MyTest) {
 *    // test code
 * }
 */
#define TEST(name)              \
  struct _TestClass##name {     \
    _TestClass##name();         \
  } _TestClass##name##Instance; \
  _TestClass##name::_TestClass##name()

// Use a namespace to hide implementation details
namespace Test::Detail {

/**
 * Make it possible to print the underlying value of class enums with ostream
 *
 * The expression typename std::enable_if<std::is_enum<T>::value,
 * std::ostream>::type decays to ostream if the type T is an enum. Otherwise,
 * the function is not generated.
 */
template <typename T>
std::ostream& operator<<(
    typename std::enable_if<std::is_enum<T>::value, std::ostream>::type& stream,
    const T& e) {
  return stream << static_cast<typename std::underlying_type<T>::type>(e);
}

/**
 * Convert anything to a string.
 */
template <typename T>
std::string toString(const T& t) {
  std::ostringstream ss;
  ss << std::setprecision(10);
  ss << t;
  return "\"" + ss.str() + "\"";
}

/**
 * Convert bools to string "true" or "false" instead of 0 and 1
 */
template <>
inline std::string toString<bool>(const bool& b) {
  return b ? "\"true\"" : "\"false\"";
}

/**
 * Comparison function for different types
 */
template <typename T>
bool isEqual(const T& t1, const T& t2) {
  return t1 == t2;
}

/**
 * Double values are equal if they differ no more than 1e-8
 */
template <>
inline bool isEqual<double>(const double& expectedValue,
                            const double& actualValue) {
  const double epsilon = 1e-4;
  const double distance = fabs(actualValue - expectedValue);
  return (distance < epsilon);
}

/**
 * Float values are equal if they differ no more than 1e-4
 */
template <>
inline bool isEqual<float>(const float& expectedValue,
                           const float& actualValue) {
  const double epsilon = 1e-4;
  const double distance = fabs(actualValue - expectedValue);
  return (distance < epsilon);
}

/**
 * This class realizes some basics of the test framework.
 * Test summary is printed in the destructor.
 * Apart from that, the class implements counting of total and failed tests,
 * comparison of floating point numbers within sensible boundaries and prints
 * the result of each test on the command line.
 */
class Test {
 public:
  /**
   * Test class is a Singleton
   */
  static Test& instance() {
    static Test test;
    return test;
  }

  /**
   * the main entry point for tests. Test two values for equality and output the
   * result.
   */
  template <typename T>
  bool check(const T& expectedValue, const T& actualValue) {
    bool testResult = isEqual(expectedValue, actualValue);
    if (testResult == true) {
      registerPassingTest();
#ifdef _OPENMP
#pragma omp critical
#endif
      std::cout << "Test successful! Expected value == actual value (="
                << toString(expectedValue) << ")" << std::endl;
    } else {
      registerFailingTest();
#ifdef _OPENMP
#pragma omp critical
#endif
      std::cout << "Error in test: expected value " << toString(expectedValue)
                << ", but actual value was " << toString(actualValue)
                << std::endl;
    }

    return testResult;
  }

 private:
  /**
   * Print a summary of all tests at the end of program execution.
   *
   * Since the Test class is a static Singleton, destruction happens when the
   * program terminates, so this is a good place to print the summary.
   */
  ~Test() {
    std::cout << "\n--------------------------------------" << std::endl;
    std::cout << "Test summary:" << std::endl;
    std::cout << "Executed tests: " << numTests_ << std::endl;
    std::cout << "Failed tests: " << numFailedTests_ << std::endl;
  }

  void registerPassingTest() { numTests_++; }

  void registerFailingTest() {
    numTests_++;
    numFailedTests_++;
  }

  /**
   * For statistics
   */
  std::atomic<int> numTests_ = 0;

  /**
   * For statistics
   */
  std::atomic<int> numFailedTests_ = 0;
};

template <typename T>
class InstanceCounterHelper {
 public:
  ~InstanceCounterHelper() {
    std::cout << "The remaining number of objects of type " << typeid(T).name()
              << " at the end of the program is " << count;
    if (count > 0)
      std::cout << " (NOT zero!)";
    std::cout << "\nThe total number of objects created was " << total
              << std::endl;
  }

  void increment() {
    count++;
    total++;
  }

  void decrement() { count--; }

 private:
  std::atomic<int> count = 0;
  std::atomic<int> total = 0;
};

}  // namespace Test::Detail

/**
 * Count the instances of a class T.
 * Result gets printed automatically at the end of the program.
 * To use it, inherit T from InstanceCounter<T>, e.g.
 * class MyClass : InstanceCounter<MyClass>
 */
template <typename T>
class InstanceCounter {
 public:
  InstanceCounter() { counter().increment(); }

  InstanceCounter(const InstanceCounter&) { counter().increment(); }

  InstanceCounter(const InstanceCounter&&) { counter().increment(); }

  virtual ~InstanceCounter() { counter().decrement(); }

  Test::Detail::InstanceCounterHelper<T>& counter() {
    static Test::Detail::InstanceCounterHelper<T> c;
    return c;
  }
};

/**
 * Check if the expected value is equal to the actual value.
 * Result is printed on the command line and at the end of the program, a
 * summary of all tests is printed.
 */
template <typename T1, typename T2>
void check(const T1& actualValue, const T2& expectedValue) {
  const T1& expectedValueCasted{
      expectedValue};  // allows conversion in general, but avoids narrowing
                       // conversion
  Test::Detail::Test::instance().check(expectedValueCasted, actualValue);
}

// allow conversion from int to double explicitely
template <>
inline void check(const double& actualValue, const int& expectedValue) {
  Test::Detail::Test::instance().check(static_cast<double>(expectedValue),
                                       actualValue);
}

/**
 * Check if the entered value is true.
 * Result is printed on the command line and at the end of the program, a
 * summary of all tests is printed.
 */
inline void check(bool a) {
  Test::Detail::Test::instance().check(true, a);
}

#endif  // VERY_SIMPLE_TEST_H

/**
 * V1.0: Creation of framework
 * V1.1: make check(bool) inline, automatically convert expected value type to
 * actual value type
 * V1.2: added possibilty to count constructions and destructions of some type
 * V1.3: tweaks on check for int and double types
 * V1.4: Adding thread safety in OpenMP programs
 * V1.5: reduce accuraccy in comparing double and float to 1e-8
 * V1.6: Increase precision for printing floating point values
 * V1.7: Put #ifdef _OPENMP around pragmas to avoid warnings when compiling
 *       without -fopenmp
 */
