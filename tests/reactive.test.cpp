#include <catch.hpp>
#include "../include/kit/kit.h"
using namespace std;
using namespace kit;

TEST_CASE("Reactive","[reactive]") {

    SECTION("as a value") {

        reactive<int> num;
        REQUIRE(num.get() == 0); // default init
        
        // set from integer rvalue
        num = 1;
        REQUIRE(num.get() == 1);

        int temp = 2;
        num = temp;
        REQUIRE(num.get() == 2);

        // copy from another reactive (implicitly deleted)
        //num = reactive<int>(3);
        //REQUIRE(num.get() == 3);
    }
    
    SECTION("as a signal") {
        bool updated = false;
        bool destroyed = false;
         
        reactive<bool> prop;
        prop.on_change.connect([&updated](const bool& b){
            updated = b;
        });
        prop.on_destroy.connect([&destroyed](){
            destroyed = true;
        });
        REQUIRE(updated == false); // still false
        prop = true;
        REQUIRE(updated == true); // now true
        
        REQUIRE(destroyed == false);
        prop.clear();
        REQUIRE(destroyed == true);
    }
    
}

