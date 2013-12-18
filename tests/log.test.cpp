#include <catch.hpp>
#include <thread>
#include "../include/kit/log/log.h"
using namespace std;

TEST_CASE("Log","[log]") {
    SECTION("thread-safe indents") {
        REQUIRE(Log::get().indents() == 0);
        {
            Log::Indent a;
            REQUIRE(Log::get().indents() == 1);

            auto t = std::thread([]{
                REQUIRE(Log::get().indents() == 0);
                REQUIRE(Log::get().num_indented_threads() == 1);
                Log::get().push_indent();
                REQUIRE(Log::get().num_indented_threads() == 2);
                Log::get().push_indent();
                REQUIRE(Log::get().indents() == 2);
                Log::get().pop_indent();
                Log::get().pop_indent();
                REQUIRE(Log::get().indents() == 0);
            });

            REQUIRE(Log::get().indents() == 1);
            REQUIRE(Log::get().num_indented_threads() == 1);
            t.join();

            Log::Indent b;

            {
                Log::Indent c;
                REQUIRE(Log::get().indents() == 3);
            }

            REQUIRE(Log::get().indents() == 2);
        }
        REQUIRE(Log::get().indents() == 0);

        // make sure all indent counts for threads auto-cleared
        REQUIRE(Log::get().num_indented_threads() == 0);
    }
}
