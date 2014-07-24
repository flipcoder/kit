#include <catch.hpp>
#include "../include/kit/kit.h"
using namespace std;
using namespace kit;

TEST_CASE("Lazy","[lazy]") {

    SECTION("as a value") {
        unsigned call_count = 0;
        lazy<int> num([&call_count]{
            ++call_count;
            return 1+1;
        });
        REQUIRE(not num.valid());
        REQUIRE(call_count == 0);
        REQUIRE(num.get() == 2);
        REQUIRE(call_count == 1);
        REQUIRE(num.valid());
        REQUIRE(num.get() == 2);
        REQUIRE(call_count == 1); // still one

        REQUIRE(num.valid());
        num.pend();
        REQUIRE(not num.valid());
        REQUIRE(not num.try_get());
        
        num.ensure();
        REQUIRE(num.valid());
        REQUIRE(num.try_get());
        REQUIRE(*num.try_get() == 2);
        
        num.set(3);
        REQUIRE(*num.try_get() == 3);
        REQUIRE(num.get() == 3);
        
        num.recache();
        REQUIRE(*num.try_get() == 2);
        REQUIRE(num.get() == 2);
    }
}

