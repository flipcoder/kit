#ifndef _KIT_COMPAT_H

#ifdef _MSC_VER
    #ifndef __WIN32__
        #define __WIN32__
    #endif
#endif

#ifdef __WIN32__
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#endif
    
#endif
