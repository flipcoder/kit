#include <functional>
#include <catch.hpp>
#include "../kit/math/common.h"
#include "../kit/log/log.h"
#include <SDL2/SDL.h>
#include "../kit/freq/animation.h"
using namespace std;

TEST_CASE("Animation","[animation]") {
    
    SECTION("frames and callbacks") {
        
        // keep track of callbacks triggered
        int callbacks = 0;
        auto callback_counter = [&callbacks] {
            ++callbacks;
        };
        //Freq::Timeline timeline;
        Animation<float> anim;
        //anim.timeline(&timeline);
        REQUIRE(floatcmp(anim.get(), 0.0f));
        
        // add callback before adding to anim
        auto frame = Frame<float>(
            1.0f,
            Freq::Time::seconds(1),
            INTERPOLATE(linear<float>)
        );
        frame.callback(callback_counter);
        anim.frame(frame);
        
        REQUIRE(anim.size() == 1);
        
        anim.frame(Frame<float>(
            10.0f,
            Freq::Time::seconds(1),
            INTERPOLATE(linear<float>)
        ));
        
        // add callback through anim's current frame ptr
        REQUIRE(anim.first_frame());
        REQUIRE(anim.last_frame());
        anim.last_frame()->callback(callback_counter);
        
        REQUIRE(anim.size() == 2);
        REQUIRE(floatcmp(anim.get(), 0.0f));
        anim.logic(Freq::Time::seconds(1));
        REQUIRE(!anim.elapsed());
        REQUIRE(anim.size() == 1);
        REQUIRE(callbacks == 1); // only 1 callback should be triggered at this point
        REQUIRE(floatcmp(anim.get(), 1.0f));
        anim.logic(Freq::Time::ms(500));
        REQUIRE(floatcmp(anim.get(), 5.5f));
        anim.logic(Freq::Time::ms(500));
        REQUIRE(floatcmp(anim.get(), 10.0f));
        REQUIRE(anim.elapsed());
        
        // check that callback was triggered twice, one for each frame completion
        REQUIRE(callbacks == 2);
    }

    SECTION("interpolation")
    {
        float start, end, x;
        
        start = Interpolation::in_sine(0.0f, 1.0f, 0.0f);
        REQUIRE(floatcmp(start, 0.0f));
        end = Interpolation::in_sine(0.0f, 1.0f, 1.0f);
        REQUIRE(floatcmp(end, 1.0f));
        
        start = Interpolation::out_sine(0.0f, 1.0f, 0.0f);
        REQUIRE(floatcmp(start, 0.0f));
        end = Interpolation::out_sine(0.0f, 1.0f, 1.0f);
        REQUIRE(floatcmp(end, 1.0f));

        x = Interpolation::in_sine(0.0f, 1.0f, 0.5f);
        REQUIRE(x > 0.29f);
        REQUIRE(x < 3.00f);
        
        x = Interpolation::out_sine(0.0f, 1.0f, 0.5f);
        REQUIRE(x > 0.70f);
        REQUIRE(x < 0.71f);
    }

    SECTION("callbacks")
    {
        bool callback = false;
        Animation<float> anim;
        auto restart = [&anim, &callback]{
            callback = false;
            anim.frame(Frame<float>(
                1.0f,
                Freq::Time::seconds(1),
                INTERPOLATE(linear<float>),
                [&callback]{
                    callback = true;
                }
            ));
        };
        
        restart();
        REQUIRE(not callback);
        anim.finish();
        REQUIRE(callback);
        
        restart();
        REQUIRE(not callback);
        anim.stop();
        REQUIRE(not callback);

        restart();
        REQUIRE(not callback);
        anim.abort();
        REQUIRE(not callback);
    }
}

