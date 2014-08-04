#include <catch.hpp>
#include "../include/kit/kit.h"
#include "../include/kit/async/async.h"
//#include "../include/kit/async/task.h"
//#include "../include/kit/async/channel.h"
//#include "../include/kit/async/multiplexer.h"
#include <atomic>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
using namespace std;

TEST_CASE("Task","[task]") {
    SECTION("empty task"){
        Task<void()> task([]{
            // empty
        });
        auto fut = task.get_future();
        task();
        REQUIRE(fut.wait_for(std::chrono::seconds(0)) == 
            std::future_status::ready);
    }
    SECTION("retrying tasks"){
        Task<void(bool)> task([](bool err){
            if(err)
                throw RetryTask();
        });
        auto fut = task.get_future();
        
        REQUIRE_THROWS(task(true));
        REQUIRE(fut.wait_for(std::chrono::seconds(0)) == 
            std::future_status::timeout);
        
        REQUIRE_NOTHROW(task(false));
        REQUIRE(fut.wait_for(std::chrono::seconds(0)) == 
            std::future_status::ready);
    }
}

TEST_CASE("Channel","[channel]") {
    SECTION("basic usage"){
        Channel<int> chan;
        REQUIRE(chan.size() == 0);
        while(true){
            try{
                chan << 42;
                break;
            }catch(const RetryTask& rt){}
        };
        REQUIRE(chan.size() == 1);
        int num = 0;
        while(true){
            try{
                chan >> num;
                break;
            }catch(const RetryTask& rt){}
        };
        REQUIRE(num == 42);
    }
    SECTION("retrying"){
        Multiplexer mx;
        Channel<int> chan;
        
        int sum = 0;
        mx.strand(0).buffer(256);
        mx.strand(0).task<void>([&chan]{
            // only way to stop this event is closing the channel from the outside
            
            chan << 1; // retries task if this blocks
            
            throw RetryTask(); // keep this event going until chan is closed
        });
        mx.strand(1).task<void>([&sum, &chan]{
            int num;
            chan >> num; // if this blocks, task is retried later
            sum += num;
            if(sum < 5)
                throw RetryTask();
            chan.close(); // done, triggers the other strand to throw
        });
        mx.finish();
        REQUIRE(sum == 5);
    }
    SECTION("nested tasks"){
        Multiplexer mx;

        bool done = false;
        {
            auto chan = make_shared<Channel<string>>();
            mx.strand(0).task<void>([&mx, chan, &done]{
                auto ping = mx.strand(0).task<void>([chan]{
                    *chan << "ping";
                    //cout << "ping" << endl;
                });
                auto pong = mx.strand(0).task<string>([chan]{
                    auto r = chan->get();
                    r[1] = 'o';
                    //cout << "pong" << endl;
                    return r;
                });
                mx.strand(0).when<void, string>(pong,[&done](future<string>& pong){
                    auto str = pong.get();
                    done = (str == "pong");
                    //cout << "done" << endl;
                });
            });
        }
        mx.finish();
        REQUIRE(done);
    }
    SECTION("get_until") {
        Multiplexer mx;
        std::string result;
        std::string msg = "hello world";
        size_t idx = 0;
        {
            auto chan = make_shared<Channel<char>>();
            mx.strand(0).task<void>([chan, &idx, &msg]{
                try{
                    *chan << msg.at(idx);
                    ++idx;
                    cout << idx << endl;
                }catch(const std::out_of_range&){
                    return;
                }
                throw RetryTask();
            });
            mx.strand(1).task<void>([&result, chan]{
                result = chan->get_until<string>(' ');
            });
        }
        mx.finish();
        REQUIRE(result == "hello");
    }
}

//TEST_CASE("TaskQueue","[taskqueue]") {

//    SECTION("basic task queue") {
//        TaskQueue<void> tasks;
//        REQUIRE(!tasks);
//        REQUIRE(tasks.empty());
//        tasks([]{});
//        REQUIRE(!tasks.empty());
//    }
        
//    SECTION("run_once w/ futures") {
//        TaskQueue<int> tasks;
//        auto fut0 = tasks([]{
//            return 0;
//        });
//        auto fut1 = tasks([]{
//            return 1;
//        });
        
//        REQUIRE(tasks.size() == 2);
//        REQUIRE(tasks);
        
//        REQUIRE(fut0.wait_for(std::chrono::seconds(0)) == 
//            std::future_status::timeout);

//        tasks.run_once();
        
//        REQUIRE(fut0.get() == 0);
//        REQUIRE(fut1.wait_for(std::chrono::seconds(0)) == 
//            std::future_status::timeout);
//        REQUIRE(!tasks.empty());
//        REQUIRE(tasks.size() == 1);
        
//        tasks.run_once();
        
//        REQUIRE(fut1.get() == 1);
//        REQUIRE(tasks.empty());
//        REQUIRE(tasks.size() == 0);
//    }

//    SECTION("run_all"){
//        TaskQueue<void> tasks;
        
//        tasks([]{});
//        tasks([]{});
//        tasks([]{});
        
//        REQUIRE(tasks.size() == 3);
        
//        tasks.run();
        
//        REQUIRE(tasks.size() == 0);
//    }

//    SECTION("nested task enqueue"){
//        TaskQueue<void> tasks;
//        auto sum = make_shared<int>(0);
//        tasks([&tasks, sum]{
//            (*sum) += 1;
//            tasks([&tasks, sum]{
//                (*sum) += 10;
//            });
//        });
//        tasks.run();
//        REQUIRE(*sum == 11);
//        REQUIRE(sum.use_count() == 1);
//    }

//}

TEST_CASE("Multiplexer","[multiplexer]") {

    SECTION("thread wait on condition"){
        Multiplexer mx;
        std::atomic<int> num = ATOMIC_VAR_INIT(0);
        mx.strand(0).task<void>([&num]{
            num = 42;
        });
        mx.strand(1).when<void>(
            [&num]{return num == 42;},
            [&num]{num = 100;}
        );
        mx.finish();
        //while(num != 100){}
        REQUIRE(num == 100);
    }
    SECTION("thread wait on future"){
        Multiplexer mx;
        Task<int()> numbers([]{
            return 42;
        });
        auto fut = numbers.get_future();
        bool done = false;
        //numbers.run();
        //REQUIRE(fut.get() == 42);
        mx.strand(0).when<void, int>(fut, [&done](std::future<int>& num){
            //done = true;
            done = (num.get() == 42);
        });
        numbers();
        mx.finish();
        REQUIRE(done);
    }
}

