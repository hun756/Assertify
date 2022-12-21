# Assertify
Assertify is a single-header C++ library for making assertions in your code. It allows you to choose how to handle an assertion failure, depending on your needs and preferences.

## Features
- Print an error message and abort the program on failure  (ASSERTIFY_ASSERT_ABORT)
- Throw an AssertionError object on  (ASSERTIFY_ASSERT_EXCEPTION)
- Store an AssertionError object in a global variable and jump back to a setjmp call in the calling function on failure (ASSERTIFY_ASSERT_EXCEPTION with ASSERTIFY_LONG_JMP_ENDABLED defined)

## Usage
 - To use Assertify, simply include the assertify.hpp header in your code. Then, use the ASSERTIFY_ASSERT_ABORT or ASSERTIFY_ASSERT_EXCEPTION macro to make an assertion: 

```cpp
#include "assertify.hpp"

int main()
{
    int x = 0;
    // Assert that x is positive and print an error message if it is not
    ASSERTIFY_ASSERT_ABORT(x > 0, "x must be positive");

    // Assert that x is positive and throw an AssertionError object if it is not
    ASSERTIFY_ASSERT_EXCEPTION(x > 0, "x must be positive");

    return 0;
}
```

## Notes
 - If you want to use the ASSERTIFY_ASSERT_EXCEPTION macro with the longjmp failure handling option, you must define the ASSERTIFY_LONG_JMP_ENDABLED macro before including the assertify.hpp header.
 - The AssertionError class is derived from std::exception and has the following member functions:
    -  const char* what() const noexcept: returns the user-defined message for the failed assertion as a const char*
    -  const char* expr_str() const: returns the string representation of the failed expression as a const char*
    -  bool expr() const: returns the result of the failed expression (should be false)
    -  const char* file() const: returns the name of the file where the assertion occurred as a const char*
    -  int line() const: returns the line number where the assertion occurred
