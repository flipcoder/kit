#include <catch.hpp>
#include "../include/kit/channel/channel.h"
using namespace std;

TEST_CASE("Channel","[channel]") {

    SECTION("basic channel") {
        Channel<void> chan;
        chan([]{});
    }
        
    SECTION("futures") {
        Channel<int> chan;
        auto fut0 = chan([]{
            return 0;
        });
        auto fut1 = chan([]{
            return 1;
        });
        REQUIRE(fut0.get() == 0);
        REQUIRE(fut1.get() == 1);
    }

}

