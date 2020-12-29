#include <catch.hpp>
#include <iostream>
#include "../kit/fs/fs.h"
using namespace std;

TEST_CASE("fs","[fs]") {
    SECTION("usage") {
        // TODO: write OS-specific tests
        
        // $HOME or $HOMEPATH
        cout << kit::homedir() << endl;
        
        // $XDG_CONFIG_HOME or $HOME/.config on Linux
        cout << kit::configdir() << endl;
        
        // $XDG_CONFIG_HOME/test or $HOME/.config/test on Linux
        cout << kit::configdir("test") << endl;
    }
}

