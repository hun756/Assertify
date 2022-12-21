#include "assertify.hpp"

int main(int argc, char *argv[])
{
    int x = 0;
    ASSERTIFY_ASSERT_EXCEPTION(x > 0, "x must be positive");
}