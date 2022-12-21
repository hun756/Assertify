/**
 * @file assertify.cpp
 * @author Mehmet Ekemen (ekemenms@gmail.com)
 *
 * @brief
 *  library is a collection of functions that are used to perform runtime checks in a program.
 *  These checks are typically used to verify that the program is behaving as expected, and to
 *  catch any potential errors or bugs that may arise.
 *
 * @version 0.1
 * @date 2022-12-21
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef ASSERTIFY_HPP_o0y1k2
#define ASSERTIFY_HPP_o0y1k2

#include <iostream>

/**
 * @brief
 *  Function that triggers an assertion failure if a given expression is false.
 *  This function is used to perform runtime checks in a program, and to identify
 *  and fix any issues that may arise.
 *
 * @param expr_str
 *  String representation of the expression being evaluated.
 *
 * @param expr
 *  Boolean value indicating the result of the expression evaluation.
 *
 * @param file
 *  Name of the source file where the assertion is being made.
 *
 * @param line
 *  Line number in the source file where the assertion is being made.
 * @param msg
 *  Optional message to include in the assertion failure output.
 */
void __Assert(const char *expr_str, bool expr, const char *file, int line,
              const char *msg)
{
    if (!expr)
    {
        std::cerr << "Assert failed:\t" << msg << "\n"
                  << "Expected:\t" << expr_str << "\n"
                  << "Source:\t\t" << file << ", Line: " << line << "\n";
        abort();
    }
}

#ifndef __CPP_AsertionError_Class

#include <exception>

/**
 * @class AssertionError
 *
 * @brief
 *  Exception class representing an assertion failure.
 *
 *  This class is thrown when an assertion fails in the program. It stores
 *  information about the failed assertion, including the expression, file, and
 *  line number where the assertion occurred, as well as a user-defined message.
 *
 *  The `what()` method can be used to get the user-defined message as a `const
 *  char*`.
 */
class AssertionError : public std::exception
{
public:
    /**
     * @brief Constructs a new AssertionError object.
     * @param expr_str String representation of the failed expression.
     * @param expr Result of the failed expression (should be `false`).
     * @param file Name of the file where the assertion occurred.
     * @param line Line number where the assertion occurred.
     * @param msg User-defined message describing the failed assertion.
     */
    AssertionError(const char *expr_str, bool expr, const char *file,
                   int line, const char *msg)
        : m_expr_str(expr_str),
          m_expr(expr),
          m_file(file),
          m_line(line),
          m_msg(msg) {}

    /**
     * @brief Returns the user-defined message for the failed assertion.
     * @return `const char*` representation of the message.
     */
    const char *what() const noexcept override
    {
        return m_msg;
    }

    // getter methods
    const char *expr_str() const { return m_expr_str; }
    bool expr() const { return m_expr; }
    const char *file() const { return m_file; }
    int line() const { return m_line; }

private:
    /** String representation of the failed expression. */
    const char *m_expr_str;
    /** Result of the failed expression (should be `false`). */
    bool m_expr;
    /** Name of the file where the assertion occurred. */
    const char *m_file;
    /** Line number where the assertion occurred. */
    int m_line;
    /** User-defined message describing the failed assertion. */
    const char *m_msg;
};

void __Assert_w_Err_Class(const char *expr_str, bool expr, const char *file, int line,
                          const char *msg)
{
    if (!expr)
    {
        throw AssertionError(expr_str, expr, file, line, msg);
    }
}

#ifdef ASSERTIFY_LONG_JMP_ENDABLED

#include <csetjmp>

namespace
{
    static std::jmp_buf s_error_handler;
    static AssertionError *s_error = nullptr;
} // anonymous namespace

/**
 * @brief Handles an assertion failure using `longjmp`.
 * @param expr_str String representation of the failed expression.
 * @param expr Result of the failed expression (should be `false`).
 * @param file Name of the file where the assertion occurred.
 * @param line Line number where the assertion occurred.
 * @param msg User-defined message describing the failed assertion.
 *
 * This function is called when an assertion fails in the program. It constructs
 * a new `AssertionError` object and stores it in a global variable, then calls
 * `longjmp` to jump back to the `setjmp` call in the calling function. The
 * `setjmp` function should be called in a block of code that is prepared to
 * handle the assertion failure.
 *
 * @warning
 *  Using longjmp to handle an assertion failure can be an effective solution in some 
 *  cases, but it is important to consider the potential drawbacks as well.
 * 
 *  One potential drawback of using longjmp is that it can make it difficult to understand 
 *  the flow of control in a program, especially if the longjmp call is buried deep in a 
 *  nested function call. This can make the program harder to debug and maintain.
 * 
 *  Another potential drawback is that longjmp can bypass normal function return and resource 
 *  deallocation, which can lead to resource leaks and other unintended side effects. 
 *  This can be particularly problematic if the longjmp call occurs in a function that 
 *  allocates resources that are not cleaned up automatically (e.g., memory allocated with new, 
 *  file handles, etc.).
 * 
 *  Finally, longjmp is not exception-safe, which means that it can leave the program in an undefined 
 *  state if an exception is thrown between the setjmp and longjmp calls.
 */
void __Assert_Long_Jmp(const char *expr_str, bool expr, const char *file, int line,
                       const char *msg)
{
    if (!expr)
    {
        s_error = new AssertionError(expr_str, expr, file, line, msg);
        std::longjmp(s_error_handler, 1);
    }
}

#define ASSERTIFY_ASSERT_EXCEPTION(expr, msg)                            \
    do                                                                   \
    {                                                                    \
        if (setjmp(s_error_handler) == 0)                                \
        {                                                                \
            __Assert_Long_Jmp(#expr, (expr), __FILE__, __LINE__, (msg)); \
        }                                                                \
        else                                                             \
        {                                                                \
            std::cerr << "Assertion failed: " << s_error->what() << "\n" \
                      << "Expected:\t" << s_error->expr_str() << "\n"    \
                      << "Source:\t\t" << s_error->file() << ", Line: "  \
                      << s_error->line() << "\n";                        \
            delete s_error;                                              \
            std::exit(1);                                                \
        }                                                                \
    } while (false)

#else

#define ASSERTIFY_ASSERT_EXCEPTION(expr, msg)                               \
    do                                                                      \
    {                                                                       \
        try                                                                 \
        {                                                                   \
            __Assert_w_Err_Class(#expr, (expr), __FILE__, __LINE__, (msg)); \
        }                                                                   \
        catch (const AssertionError &e)                                     \
        {                                                                   \
            std::cerr << "Assertion failed: " << e.what() << "\n"           \
                      << "Expected:\t" << e.expr_str() << "\n"              \
                      << "Source:\t\t" << e.file() << ", Line: "            \
                      << e.line() << "\n";                                  \
            std::exit(1);                                                   \
        }                                                                   \
    } while (false)

#endif // ASSERTIFY_LONG_JMP

#endif // __CPP_AsertionError_Class

#endif /* End of include guard: ASSERTIFY_HPP_o0y1k2 */