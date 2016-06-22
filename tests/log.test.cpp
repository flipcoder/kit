#include <catch.hpp>
#include <thread>
#include "../kit/log/log.h"
using namespace std;

TEST_CASE("Log","[log]") {
    SECTION("thread-safe indents") {
        REQUIRE(Log::get().indents() == 0);
        {
            Log::Indent a;
            REQUIRE(Log::get().indents() == 1);

            auto t = std::thread([]{
                REQUIRE(Log::get().indents() == 0);
                REQUIRE(Log::get().num_threads() == 1);
                Log::get().push_indent();
                REQUIRE(Log::get().num_threads() == 2);
                Log::get().push_indent();
                REQUIRE(Log::get().indents() == 2);
                Log::get().pop_indent();
                Log::get().pop_indent();
                REQUIRE(Log::get().indents() == 0);
            });

            REQUIRE(Log::get().indents() == 1);
            REQUIRE(Log::get().num_threads() == 1);
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
        REQUIRE(Log::get().num_threads() == 0);
    }

    SECTION("emit"){
        //auto ll = Log::get().lock();
        //Log::Capturer c;
        //LOG("hello");
        //REQUIRE(Log::get().emit() == "hello");
    }
    SECTION("consume prefix"){
        // purpose
        REQUIRE(Log::consume_prefix("(example.cpp:123): data") == "data");

        // incomplete prefixes
        REQUIRE(Log::consume_prefix("(") == "(");
        REQUIRE(Log::consume_prefix("()") == "()");
        REQUIRE(Log::consume_prefix("():") == "():");
        REQUIRE(Log::consume_prefix("(blah):") == "(blah):");
        REQUIRE(Log::consume_prefix("(blah:123):") == "(blah:123):");
        
        // complex prefix
        REQUIRE(Log::consume_prefix("(): ") == "");

        // nested colons are kept
        REQUIRE(Log::consume_prefix("(blah): ") == "");
        REQUIRE(Log::consume_prefix("(blah:123): ") == "");
        REQUIRE(Log::consume_prefix("(blah): data") == "data");
        REQUIRE(Log::consume_prefix("(:::): data") == "data");
        
        // multi-line
        REQUIRE(Log::consume_prefix("(): \n(): ") == "\n");
        REQUIRE(Log::consume_prefix("(): \n\n(): ") == "\n\n");
        REQUIRE(Log::consume_prefix("(): foo\n(): bar") == "foo\nbar");
        REQUIRE(Log::consume_prefix("foo\n(): \n(): \nbar") == "foo\n\n\nbar");
    }

    SECTION("signals"){
        int happened = 0;
        {
            boost::signals2::scoped_connection connection = Log::get().on_log.connect([&](std::string, Log::Level lev){
                ++happened;
            });
            REQUIRE(happened == 0);
            LOG("test");
            REQUIRE(happened == 1);
        }
        REQUIRE(happened == 1);
        LOG("test2");
        REQUIRE(happened == 1);
    }
}

