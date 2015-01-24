solution("kit")
    configurations {"debug", "release"}

    targetdir("bin")

    configuration "debug"
        defines { "DEBUG" }
        flags { "Symbols" }
    configuration "release"
        defines { "NDEBUG" }
        flags { "OptimizeSpeed" }

    project("echo")
        kind("ConsoleApp")
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
        files {
            "src/echo.cpp",
            "../include/**.cpp",
            "../include/**.inl"
        }

        defines { "BACKWARD_HAS_BFD=1" }

        includedirs {
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

    project("stability")
        kind("ConsoleApp")
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
        files {
            "src/stability.cpp",
            "../include/**.cpp",
            "../include/**.inl"
        }

        defines { "BACKWARD_HAS_BFD=1" }

        includedirs {
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

