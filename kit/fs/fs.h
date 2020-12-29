#ifndef FS_H_C8OF7L4K
#define FS_H_C8OF7L4K

#include <string>
#include <boost/filesystem/path.hpp>
#include "../log/log.h"

#ifdef _WIN32
    static const std::string PATH_SEP = "\\";
#else
    static const std::string PATH_SEP = "/";
#endif

namespace kit {
    
    inline std::string homedir() {
        const char* homedir = getenv("HOME");
        if(!homedir)
            homedir = getenv("HOMEPATH");
        if(!homedir)
            K_ERROR(GENERAL, "Unable to locate home directory.");
        return homedir;
    }

    inline std::string configdir(std::string apppath = std::string()) {
        std::string r;
        bool add_config=false;
        const char* cdir = getenv("XDG_CONFIG_PATH");
        if(!cdir){
            cdir = getenv("HOME");
            add_config=true;
        }
        if(!cdir){
            cdir = getenv("HOMEPATH");
            add_config=true;
        }
        if(!cdir){
            K_ERROR(GENERAL, "Unable to locate home directory.");
        }
        if(add_config)
        {
#ifdef _WIN32
            r = std::string(cdir);
#else
            r = std::string(cdir) + PATH_SEP + ".config";
#endif
        }
        else
            r = cdir;
        if(not apppath.empty())
            return r + PATH_SEP + apppath;
        return r;
    }
}

#endif

