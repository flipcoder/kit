#include <catch.hpp>
#include "../include/kit/reactive/reactive.h"
#include "../include/kit/reactive/signal.h"
using namespace std;
using namespace kit;

TEST_CASE("Signal","[signal]") {
    SECTION("basic") {
        unsigned i = 0;
        signal<void()> sig;
        REQUIRE(sig == false);
        sig.connect([&i]{++i;});
        REQUIRE(sig == true);
        REQUIRE(i == 0);
        sig();
        REQUIRE(i == 1);
        sig();
        REQUIRE(i == 2);
        REQUIRE(sig == true);
        sig.clear();
        REQUIRE(sig == false);
        sig();
        REQUIRE(i == 2);
    }
    SECTION("accumulation") {
        signal<int()> sig;
        sig.connect([]{ return 1; });
        sig.connect([]{ return 2; });
        sig.connect([]{ return 3; });
        auto vals = sig.accumulate();
        REQUIRE((vals == vector<int>{1,2,3}));
    }
    SECTION("reentrant") {
        signal<void()> sig;
        unsigned call_count = 0;
        sig.connect([&sig, &call_count]{
            sig.connect([&sig, &call_count]{
                ++call_count;
            });
        });
        REQUIRE(call_count == 0);
        sig();
        REQUIRE(call_count == 0);
        sig();
        REQUIRE(call_count == 1);
    }
    //SECTION("sync & pend") {
    //}
}

TEST_CASE("Reactive","[reactive]") {

    SECTION("as a value") {

        reactive<int> num;
        REQUIRE(num.get() == *num); // default init
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
        //bool destroyed = false;
         
        reactive<bool> prop;
        prop.on_change.connect([&updated](const bool& b){
            updated = b;
        });
        //prop.on_destroy.connect([&destroyed](){
        //    destroyed = true;
        //});
        REQUIRE(updated == false); // still false
        prop = true;
        REQUIRE(updated == true); // now true
        
        //REQUIRE(destroyed == false);
        //prop.clear();
        //REQUIRE(destroyed == true);
    }
    
}

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

