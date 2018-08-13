solution("kit")
    configurations {"debug", "release"}

    targetdir("bin")

    defines {
        "GLM_FORCE_CTOR_INIT",
        "GLM_ENABLE_EXPERIMENTAL",
        "GLM_FORCE_RADIANS",
        "NOMINMAX"
    }

    configuration "debug"
        defines { "DEBUG" }
        flags { "Symbols" }
    configuration "release"
        defines { "NDEBUG" }
        flags { "OptimizeSpeed" }

    language("C++")
    links {
        "pthread",
        "boost_thread",
        "SDL2",
        "boost_system",
        "boost_regex",
        "boost_filesystem",
        "boost_coroutine",
        "jsoncpp"
    }
    defines { "BACKWARD_HAS_BFD=1" }
    includedirs {
        "../lib/local_shared_ptr",
        "../vendor/include/",
        "/usr/include/cpp-netlib/"
    }
    
    configuration {"debug"}
        links {
            "bfd",
            "iberty"
        }
        linkoptions { "`llvm-config --libs core` `llvm-config --ldflags`" }
    configuration {}

    configuration { "gmake" }
        --buildoptions { "-std=c++11",  "-pedantic", "-Wall", "-Wextra" }
        buildoptions { "-std=c++11" }
        configuration { "macosx" }
            buildoptions { "-U__STRICT_ANSI__", "-stdlib=libc++" }
            linkoptions { "-stdlib=libc++" }
    configuration {}
    
    files { "../kit/**.cpp" }
    
    project("echo")
        kind("ConsoleApp")
        files { "src/echo.cpp" }

    project("stability")
        kind("ConsoleApp")
        files { "src/stability.cpp" }

    project("chat")
        kind("ConsoleApp")
        files { "src/chat.cpp" }

