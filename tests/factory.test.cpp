#include <catch.hpp>
#include "../include/kit/factory/factory.h"
using namespace std;

TEST_CASE("Factory","[factory]") {

    SECTION("basic usage") {
        Factory<std::string, std::string> factory;
    }

}
