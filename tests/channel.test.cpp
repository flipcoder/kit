#include <catch.hpp>
#include "../include/kit/channel/channel.h"
using namespace std;

TEST_CASE("Channel","[channel]") {

    SECTION("basic channel") {
        Channel<void> chan;
        REQUIRE(chan.empty());
        chan([]{});
        REQUIRE(!chan.empty());
    }
        
    SECTION("run_once w/ futures") {
        Channel<int> chan;
        auto fut0 = chan([]{
            return 0;
        });
        auto fut1 = chan([]{
            return 1;
        });
        
        REQUIRE(chan.size() == 2);
        
        REQUIRE(fut0.wait_for(std::chrono::milliseconds(100)) == 
            std::future_status::timeout);

        chan.run_once();
        
        REQUIRE(fut0.get() == 0);
        REQUIRE(fut1.wait_for(std::chrono::milliseconds(100)) == 
            std::future_status::timeout);
        REQUIRE(!chan.empty());
        REQUIRE(chan.size() == 1);
        
        chan.run_once();
        
        REQUIRE(fut1.get() == 1);
        REQUIRE(chan.empty());
        REQUIRE(chan.size() == 0);
    }

    SECTION("run_all"){
        Channel<void> chan;
        
        chan([]{});
        chan([]{});
        chan([]{});
        
        REQUIRE(chan.size() == 3);
        
        chan.run_all();
        
        REQUIRE(chan.size() == 0);
    }
}

