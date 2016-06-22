#include "../../kit/async/async.h"
#include "../../kit/log/log.h"
#include <chrono>
using namespace std;

int main(int argc, char** argv)
{
    srand(time(NULL));
    for(unsigned i=0;i<MX.size();++i)
        MX[i].frequency(1.0f);
    
    for(unsigned i=0;i<10;++i)
    {
        auto c = rand() % MX.size();
        MX[c].coro<void>([i,c]{
            for(;;){
                YIELD();
            }
        });
    }
    
    return 0;
}

