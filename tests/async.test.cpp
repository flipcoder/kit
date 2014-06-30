#include <catch.hpp>
#include "../include/kit/async/taskqueue.h"
#include "../include/kit/async/channel.h"
#include "../include/kit/async/multiplexer.h"
#include <atomic>
using namespace std;

TEST_CASE("Channel","[channel]") {
    SECTION("basic usage"){
        Channel<int> chan;
        REQUIRE(chan.size() == 0);
        chan << 42;
        REQUIRE(chan.size() == 1);
        int num = 0;
        bool b = (chan >> num);
        REQUIRE(b);
        REQUIRE(num == 42);
    }
}

TEST_CASE("TaskQueue","[taskqueue]") {

    SECTION("basic task queue") {
        TaskQueue<void> tasks;
        REQUIRE(!tasks);
        REQUIRE(tasks.empty());
        tasks([]{});
        REQUIRE(!tasks.empty());
    }
        
    SECTION("run_once w/ futures") {
        TaskQueue<int> tasks;
        auto fut0 = tasks([]{
            return 0;
        });
        auto fut1 = tasks([]{
            return 1;
        });
        
        REQUIRE(tasks.size() == 2);
        REQUIRE(tasks);
        
        REQUIRE(fut0.wait_for(std::chrono::seconds(0)) == 
            std::future_status::timeout);

        tasks.run_once();
        
        REQUIRE(fut0.get() == 0);
        REQUIRE(fut1.wait_for(std::chrono::seconds(0)) == 
            std::future_status::timeout);
        REQUIRE(!tasks.empty());
        REQUIRE(tasks.size() == 1);
        
        tasks.run_once();
        
        REQUIRE(fut1.get() == 1);
        REQUIRE(tasks.empty());
        REQUIRE(tasks.size() == 0);
    }

    SECTION("run_all"){
        TaskQueue<void> tasks;
        
        tasks([]{});
        tasks([]{});
        tasks([]{});
        
        REQUIRE(tasks.size() == 3);
        
        tasks.run();
        
        REQUIRE(tasks.size() == 0);
    }

    SECTION("nested task enqueue"){
        TaskQueue<void> tasks;
        auto sum = make_shared<int>(0);
        tasks([&tasks, sum]{
            (*sum) += 1;
            tasks([&tasks, sum]{
                (*sum) += 10;
            });
        });
        tasks.run();
        REQUIRE(*sum == 11);
        REQUIRE(sum.use_count() == 1);
    }

}

TEST_CASE("Multiplexer","[multiplexer]") {
    SECTION("thread wait on condition"){
        Multiplexer mx;
        std::atomic<int> num = ATOMIC_VAR_INIT(0);
        mx.strand(0).task([&num]{
            num = 42;
        });
        mx.strand(1).when(
            [&num]{return num == 42;},
            [&num]{num = 100;}
        );
        mx.finish();
        //while(num != 100){}
        REQUIRE(num == 100);
    }
    SECTION("thread wait on future"){
        Multiplexer mx;
        TaskQueue<int> numbers;
        auto fut = numbers([]{
            return 42;
        });
        bool done = false;
        //numbers.run();
        //REQUIRE(fut.get() == 42);
        mx.strand(0).when<int>(fut, [&done](int num){
            //done = true;
            done = (num == 42);
        });
        numbers.run();
        mx.finish();
        REQUIRE(done);
    }
}

