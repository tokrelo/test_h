# TEST_H

This is an *extremely simple* test framework for C++.

## Usage

### Include test framework in your code
Just copy the file to your code repository and add `#include "test.h"` in your code. Nothing to be compiled or linked, just one header included.

### Writing tests
There is a single function `check(actual, expected)` that tests whether *actual* matches *expected*. The result of the check is printed to the command line. A summary of all performed tests is printed at the end of the program execution.

### Test Macro
There also is a TEST macro. It is basically a function that is executed automatically, so you don't need to call anything from main. Example usage
```
TEST(MyTest) {
  check(4, 5);
}
```


