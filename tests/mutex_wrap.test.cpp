#include <catch.hpp>
#include "../include/kit/kit.h"
using namespace std;
using namespace kit;

TEST_CASE("Mutex wrap","[mutex_wrap]") {

    SECTION("basic usage") {

        mutex_wrap<int> num(0);
        REQUIRE(num == 0);
        num.with<void>([](int& num){
            num = 1;
        });
        REQUIRE(num == 1);
        
    }
}

