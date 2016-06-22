#include <catch.hpp>
#include "../kit/kit.h"
using namespace std;

TEST_CASE("Slice","[slice]") {
    SECTION("slice") {
        vector<int> v = {1,2,3,4,5};
        REQUIRE(kit::slice(v,1,4) == (vector<int>{2,3,4}));
        REQUIRE(kit::slice(v,1,10) == (vector<int>{2,3,4,5}));
        REQUIRE(kit::slice(v,1,10) == (vector<int>{2,3,4,5}));
        REQUIRE(kit::slice(v,1,-1) == (vector<int>{2,3,4}));
        REQUIRE(kit::slice(v,-3,-1) == (vector<int>{3,4}));
        
        REQUIRE(kit::slice(v,0, 10, 3) == (vector<int>{1,4}));
    }
}

