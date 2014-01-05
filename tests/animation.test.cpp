#include <functional>
#include <catch.hpp>
#include "../include/kit/math/common.h"
#include <SDL2/SDL.h>
#include "../include/kit/freq/animation.h"
using namespace std;

TEST_CASE("Animation","[animation]") {
    
    SECTION("frames and callbacks") {
        
        // keep track of callbacks triggered
        int callbacks = 0;
        auto callback_counter = [&callbacks] {
            ++callbacks;
        };
        Freq::Timeline timeline;
        Animation<float> anim;
        anim.timeline(&timeline);
        REQUIRE(floatcmp(anim.get(), 0.0f));
        
        // add callback before adding to anim
        auto frame = Frame<float>(
            1.0f,
            Freq::Time::seconds(1),
            INTERPOLATE(linear)
        );
        frame.callback(callback_counter);
        anim.frame(frame);
        
        REQUIRE(anim.size() == 1);
        
        anim.frame(Frame<float>(
            10.0f,
            Freq::Time::seconds(1),
            INTERPOLATE(linear)
        ));
        
        // add callback through anim's current frame ptr
        REQUIRE(anim.first_frame());
        REQUIRE(anim.last_frame());
        anim.last_frame()->callback(callback_counter);
        
        REQUIRE(anim.size() == 2);
        REQUIRE(floatcmp(anim.get(), 0.0f));
        timeline.logic(Freq::Time::seconds(1));
        REQUIRE(!anim.elapsed());
        REQUIRE(anim.size() == 1);
        REQUIRE(callbacks == 1); // only 1 callback should be triggered at this point
        REQUIRE(floatcmp(anim.get(), 1.0f));
        timeline.logic(Freq::Time::ms(500));
        REQUIRE(floatcmp(anim.get(), 5.5f));
        timeline.logic(Freq::Time::ms(500));
        REQUIRE(floatcmp(anim.get(), 10.0f));
        REQUIRE(anim.elapsed());
        
        // check that callback was triggered twice, one for each frame completion
        REQUIRE(callbacks == 2);
    }
}

