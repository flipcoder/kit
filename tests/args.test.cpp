#include <catch.hpp>
#include "../include/kit/args/args.h"
using namespace std;

TEST_CASE("Args","[args]") {

    SECTION("empty") {
        Args args;
        REQUIRE(args.empty());
        REQUIRE(args.size() == 0);
    }

}

