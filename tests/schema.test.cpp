#include <catch.hpp>
#include <memory>
#include "../kit/meta/schema.h"
using namespace std;

TEST_CASE("Schema","[schema]") {

    SECTION("load from string") {
        
        auto schema = make_shared<Schema>(
            make_shared<Meta>(
                MetaFormat::JSON,
                R"({})"
            )
        );
        REQUIRE(schema->meta());
        REQUIRE(schema->meta().get());
        REQUIRE(schema->meta()->empty());
        
        schema = make_shared<Schema>(
            make_shared<Meta>(
                MetaFormat::JSON,
                R"({
                    "foo": "bar",
                    "a": [
                        "a",
                        "b",
                        "c"
                    ],
                    "b": {
                        "a": 1,
                        "b": 2,
                        "b": 3
                    }
                })"
            )
        );
        REQUIRE(schema->meta()->size() == 3);
        REQUIRE(schema->meta()->at<string>("foo") == "bar");
    }
    
    SECTION("validate") {
    
        auto schema = make_shared<Schema>(
            make_shared<Meta>(
                MetaFormat::JSON,
                R"({
                    "foo": {
                        ".values": [
                            "a",
                            "b",
                            "c"
                        ]
                    }
                })"
            )
        );

        // diff mutex type, for variety
        auto test = make_shared<MetaMT>(
            MetaFormat::JSON,
            R"({
                "foo": "a"
            })"
        );
        
        REQUIRE_NOTHROW(schema->validate(test));
        REQUIRE(schema->check(test));
        test->set<string>("foo", "d");
        {
            // don't logging the error output (will still throw)
            Log::Silencer ls(Log::Silencer::ERRORS);
            REQUIRE_THROWS(schema->validate(test));
        }
        REQUIRE_FALSE(schema->check(test));
    }
}

