#include <catch.hpp>
#include "../include/kit/channel/channel.h"
using namespace std;

TEST_CASE("Channel","[channel]") {

    SECTION("single-channel multiplexer") {
        Multiplexer<void> plex;
        REQUIRE(plex.size() == 1);
        plex([]{});
    }
        
    SECTION("multiplexer futures") {
        Multiplexer<int> plex(2);
        auto fut0 = plex(0, []{
            return 0;
        });
        auto fut1 = plex(1, []{
            return 1;
        });
        REQUIRE(fut0.get() == 0);
        REQUIRE(fut1.get() == 1);
        REQUIRE(plex.size() == 2);
    }

}

