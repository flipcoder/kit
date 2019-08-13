#include <catch.hpp>
#include "../kit/args/args.h"
#include "../kit/log/log.h"
using namespace std;

TEST_CASE("Args","[args]") {

    SECTION("empty") {
        Args args;
        REQUIRE(args.empty());
        REQUIRE(args.size() == 0);
    }
    
    SECTION("at") {
        Args args;

        args = Args(vector<string>{"a","b","c"});
        
        REQUIRE(args.at(0) == "a");
        REQUIRE(args.at(1) == "b");
        REQUIRE(args.at(2) == "c");
        REQUIRE(args.at(3) == "");
        REQUIRE(args.at(-1) == "c");
        REQUIRE(args.at(-2) == "b");
        REQUIRE(args.at(-3) == "a");
        REQUIRE(args.at(-4) == "");
    }
    
    SECTION("has") {
        // empty
        Args args;
        REQUIRE(not args.has("foobar"));
        
        // single arg
        args = Args(vector<string>{"foobar"});
        REQUIRE(args.has("foobar"));
        REQUIRE(not args.has("foo"));
        
        // multiple args
        args = Args(vector<string>{"foo", "bar"});
        REQUIRE(args.has("foo"));
        REQUIRE(args.has("bar"));
        REQUIRE(not args.has("baz"));

        // switches
        args = Args();
        REQUIRE(not args.has('v', "verbose"));
        args = Args(vector<string>{"--verbose"});
        REQUIRE(args.has('v', "verbose")); // full word
        REQUIRE(not args.has('n', "nope"));
        args = Args(vector<string>{"-v"});
        REQUIRE(args.has('v', "verbose")); // single char
        REQUIRE(not args.has('n', "nope"));

        // multiple char switches
        args = Args(vector<string>{"-abc"}, "-a -b -c");
        REQUIRE(args.has('a', "achar"));
        REQUIRE(args.has('b', "bchar"));
        REQUIRE(args.has('c', "cchar"));
        args = Args(vector<string>{"-ac"}, "-a -b -c");
        REQUIRE(args.has('a', "achar"));
        REQUIRE(not args.has('b', "bchar"));
        REQUIRE(args.has('c', "cchar"));
    }

    SECTION("key-value") {
        Args args;
        REQUIRE_NOTHROW(args = Args(vector<string>{"--foo=bar"}));
        REQUIRE(args.value("foo") == "bar");
        REQUIRE(args.value_or("foo","baz") == "bar");
        REQUIRE(args.value_or("bin","baz") == "baz");
    }
    
    SECTION("expected") {
        Args args;
        
        REQUIRE_NOTHROW(args = Args(vector<string>{"--foo"}, "-f --foo"));
        REQUIRE(args.has('f',"foo"));
        REQUIRE(args.has("--foo"));
        REQUIRE(not args.has("-f"));
        
        REQUIRE_NOTHROW(args = Args(vector<string>{"-f"}, "-f --foo"));
        REQUIRE(args.has('f',"foo"));
        REQUIRE(not args.has("--foo"));
        REQUIRE(args.has("-f"));
        
        {
            Log::Silencer ls;
            REQUIRE_THROWS(args = Args(vector<string>{"--invalid"}, "-f --foo"));
        }
        
        REQUIRE_NOTHROW(Args(vector<string>{"-ab"}, "-a -b"));
        {
            Log::Silencer ls;
            // -c invalid
            REQUIRE_THROWS(Args(vector<string>{"-abc"}, "-a -b"));
        }
    }
}

