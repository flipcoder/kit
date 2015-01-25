#include <catch.hpp>
#include "../include/kit/meta/meta.h"
using namespace std;

TEST_CASE("Meta","[meta]") {

    SECTION("empty") {
        auto m = make_shared<Meta>();
        REQUIRE(m);
        REQUIRE(!*m);
        REQUIRE(m->empty());
        REQUIRE(m->size() == 0);
    }
    
    SECTION("set") {
        auto m = make_shared<Meta>();
        m->set<int>("one", 1);

        REQUIRE(*m);
        REQUIRE(!m->empty());
        REQUIRE(m->size() == 1);

        m->set<string>("two", "2");
        REQUIRE(m->size() == 2);
        REQUIRE(m->key_count() == 2);
    }

    SECTION("add") {
        auto m = make_shared<Meta>();

        REQUIRE(m->add(make_shared<Meta>()) == 0);
        REQUIRE(m->add(make_shared<Meta>()) == 1);
        REQUIRE(m->size() == 2);
        REQUIRE(m->key_count() == 0);
    }
    
    SECTION("ensure") {
        auto m = make_shared<Meta>();

        REQUIRE(m->ensure<int>("a", 1) == 1);
        REQUIRE(m->ensure<int>("b", 2) == 2);
        REQUIRE(m->ensure<int>("a", 3) == 1);
        REQUIRE(m->at<int>("a") == 1);
        REQUIRE(m->size() == 2);

        // TODO: type conflicts
        REQUIRE(m->ensure<string>("a", "test") == "test");
    }

    SECTION("remove") {

        SECTION("at index") {
            auto m = make_shared<Meta>();
            m->add(1);
            m->add(2); // index 1
            m->add(3);
            REQUIRE(m->is_array());
            m->remove(1);
            REQUIRE(m->is_array());
            REQUIRE(m->at<int>(0) == 1);
            REQUIRE(m->at<int>(1) == 3);
            REQUIRE(m->size() == 2);
            REQUIRE_THROWS_AS(m->at<int>(2), std::out_of_range);
            m->remove(0);
            REQUIRE(m);
            m->remove(0);
            REQUIRE(!*m);
            REQUIRE_THROWS_AS(m->remove(0), std::out_of_range);
        }

        SECTION("pop") {
            SECTION("front") {
                auto m = make_shared<Meta>();
                m->add<int>(1);
                m->add<int>(2);
                m->pop_front();
                REQUIRE(m->at<int>(0) == 2);
                REQUIRE(m->size() == 1);
                m->pop_front();
                REQUIRE(m->empty());
                REQUIRE_THROWS_AS(m->pop_front(), std::out_of_range);
            }
            SECTION("back") {
                auto m = make_shared<Meta>();
                m->add<int>(1);
                m->add<int>(2);
                m->pop_back();
                REQUIRE(m->at<int>(0) == 1);
                REQUIRE(m->size() == 1);
                m->pop_back();
                REQUIRE(m->empty());
                REQUIRE_THROWS_AS(m->pop_back(), std::out_of_range);
            }
        }

        SECTION("by key") {
            auto m = make_shared<Meta>();
            m->set("one" ,1);
            m->set("two", 2);
            m->set("three", 3);
            REQUIRE(m->is_map());
            m->remove("two");
            REQUIRE(m->is_map());
            REQUIRE_THROWS_AS(m->at<int>("two"), std::out_of_range);
            REQUIRE(m->at<int>(0) == 1);
            REQUIRE(m->at<int>(1) == 3);
            m->remove("one");
            m->remove("three");
            REQUIRE_THROWS_AS(m->remove("two"), std::out_of_range);
            REQUIRE_THROWS_AS(m->remove("four"), std::out_of_range);
        }
    }

    SECTION("types") {
        {
            auto m = make_shared<Meta>();
            m->set("nullptr", nullptr);
            m->set("nullmeta", std::shared_ptr<Meta>());
            REQUIRE(not m->at<nullptr_t>("nullptr"));
            REQUIRE(not m->at<shared_ptr<Meta>>("nullmeta"));
        }
    }
 
    //SECTION("copying and assignment") {
    //    // n/a
    //}

    SECTION("clear") {
        auto m = make_shared<Meta>();
        m->set<int>("one",1);
        m->set<string>("two","2");

        m->clear();
        REQUIRE(m);
        REQUIRE(!*m);
        REQUIRE(m->empty());
        REQUIRE(m->size() == 0);
    }
       
    SECTION("at") {
        auto m = make_shared<Meta>();
        m->set<int>("one", 1);
        m->set<string>("two", "2");
        m->add(make_shared<Meta>());

        REQUIRE(m->at<int>("one") == 1);
        REQUIRE(m->at<int>(0) == 1);
        REQUIRE(m->at<string>(1) == "2");
        REQUIRE_THROWS_AS(m->at<string>(3), std::out_of_range);
        REQUIRE_THROWS_AS(m->at<string>("one"), boost::bad_any_cast);
    }

    SECTION("parent") {
        auto m = make_shared<Meta>();
        REQUIRE(!m->parent());
        REQUIRE(m->root() == m);
        auto c1 = make_shared<Meta>();
        m->add(c1);
        REQUIRE(c1->parent() == m);
    }

    SECTION("iterate") {
        SECTION("map") {
            string json = "{\"one\":1,\"two\":2,\"three\": 3}";
            auto m = make_shared<Meta>(MetaFormat::JSON, json);
            int i = 0;
            m->each([&](
                const shared_ptr<Meta>& parent,
                MetaElement& e,
                unsigned level
            ){
                REQUIRE(parent);
                REQUIRE(parent == m);
                //REQUIRE(level == 0);
                REQUIRE(!e.key.empty());
                REQUIRE(json.find("\"" + e.key + "\"") != string::npos);
                
                // key order not required
                //REQUIRE(i == boost::any_cast<int>(e.value));
                
                ++i;
                return MetaLoop::STEP;
            });
            REQUIRE(i == m->size());
        }
        SECTION("array") {
        }
        SECTION("recursive") {
        }
        SECTION("by type") {
        }
    }

    SECTION("each") {
    }

    SECTION("merge") {
    }

    SECTION("serialization") {

        SECTION("objects") {
            auto m = make_shared<Meta>(MetaFormat::JSON,"{\"one\":1}");
            REQUIRE(m->at<int>("one") == 1);
            m->clear();

            m = make_shared<Meta>();
            m->deserialize(MetaFormat::JSON,"{\"one\":1}");
            REQUIRE(m->at<int>("one") == 1);
            
            string data = m->serialize(MetaFormat::JSON);
            m->clear();
            REQUIRE(m->empty());
            m->deserialize(MetaFormat::JSON,data);
            REQUIRE(not m->empty());
            REQUIRE(m->at<int>("one") == 1);
        }

        SECTION("arrays") {
            auto m = make_shared<Meta>(MetaFormat::JSON,"{\"numbers\":[1,2,3]}");
            auto a = m->at<std::shared_ptr<Meta>>(0);
            REQUIRE(a);
            REQUIRE(a->size() == 3);
            REQUIRE(a->at<int>(0) == 1);
            REQUIRE(a->at<int>(1) == 2);
            REQUIRE(a->at<int>(2) == 3);
        }

        SECTION("overwriting") {
            auto m = make_shared<Meta>(MetaFormat::JSON,"{\"one\":1}");
            REQUIRE(m->at<int>("one") == 1);
            m->deserialize(MetaFormat::JSON,"{\"two\":2}");
            //REQUIRE(m->size() == 1); // decide this behavior (flag?)
            //REQUIRE_THROWS_AS(m->at<int>("one"), std::out_of_range);
            REQUIRE(m->at<int>("two") == 2);
        }

        SECTION("nested") {
            
        }

    }
}

