#include <catch.hpp>
#include <memory>
#include "../include/kit/schema/schema.h"
using namespace std;

TEST_CASE("Schema","[schema]") {

    SECTION("empty") {
        auto schema = make_shared<Schema<>>();
    }
    
}

