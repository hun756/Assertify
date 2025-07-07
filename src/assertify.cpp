#include <assertify/assertify.hpp>

namespace assertify
{
namespace detail
{

thread_local basic_memory_pool tl_pool;

} // namespace detail
} // namespace assertify