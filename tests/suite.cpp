#include <exception>
namespace std {
inline bool uncaught_exception() noexcept(true) {return current_exception() != nullptr;}
}

#define CATCH_CONFIG_MAIN
#include <catch.hpp>
