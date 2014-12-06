#include <catch.hpp>
#include "../include/kit/net/net.h"

TEST_CASE("Socket","[socket]") {
    SECTION("tcp"){
        Socket s(Socket::TCP);
        s.bind();
        REQUIRE(not s);
        REQUIRE_NOTHROW(s.open());
        //REQUIRE(s.connect("127.0.0.1", 1337));
        REQUIRE(s);
        //REQUIRE(not s.select());
        REQUIRE_NOTHROW(s.close());
        REQUIRE(not s);
    }
    SECTION("udp"){
        Socket s(Socket::UDP);
        REQUIRE(not s);
        REQUIRE_NOTHROW(s.open());
        REQUIRE(s);
        //REQUIRE(not s.select());
        REQUIRE_NOTHROW(s.close());
        REQUIRE(not s);
    }
}

