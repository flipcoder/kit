#include <catch.hpp>
#include <iostream>
#include "../kit/fs/fs.h"
using namespace std;

TEST_CASE("fs","[fs]") {
    SECTION("usage") {
        cout << fs::homedir() << endl;
        cout << fs::configdir() << endl;
        cout << fs::configdir("test") << endl;
    }
}

