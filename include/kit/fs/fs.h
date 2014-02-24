#ifndef FS_H_C8OF7L4K
#define FS_H_C8OF7L4K

#include <string>
#include <boost/filesystem/path.hpp>
#include "../log/log.h"

namespace fs {

    inline std::string homedir() {
        const char* homedir = getenv("HOME");
        if(!homedir)
            homedir = getenv("HOMEPATH");
        if(!homedir)
            ERROR(GENERAL, "Unable to locate home directory.");
        return homedir;
    }
}

#endif

