#include <catch.hpp>
#include "../kit/cache/cache.h"
using namespace std;

struct Object
{
    Object(const std::tuple<std::string, ICache*>& args):
        m_Value(std::get<0>(args))
    {}
    std::string value() const {
        return m_Value;
    }
    std::string m_Value;
};

TEST_CASE("Cache","[cache]") {

    SECTION("basic usage") {
        Cache<Object, string> cache;
        cache.register_class<Object>();
        cache.register_resolver(
            [](tuple<string, ICache*>) {
                return 0;
            }
        );
        
        auto hi_ptr = cache.cache("hi"); // hold shared_ptr to resource
        REQUIRE(cache.size()==1);
        
        cache.cache("hi"); // reuse old resource
        REQUIRE(cache.size()==1);
        
        cache.cache("hello"); // new resource
        REQUIRE(cache.size()==2);
        REQUIRE(!cache.empty());
        
        cache.optimize();
        REQUIRE(hi_ptr->value() == "hi");
        REQUIRE(cache.size()==1);
        
        hi_ptr = nullptr;
        REQUIRE(cache.size()==1);
        cache.optimize();
        
        REQUIRE(cache.size()==0);
        REQUIRE(cache.empty());
    }
}
