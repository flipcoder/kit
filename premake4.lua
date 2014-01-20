--newoption {
--    trigger = "test",
--    description = "builds unit tests"
--}

solution("kit")
    configurations {"Debug", "Release"}

    targetdir("bin")

    configuration "debug"
        defines { "DEBUG" }
        flags { "Symbols" }
    configuration "release"
        defines { "NDEBUG" }
        flags { "OptimizeSpeed" }

    --project("kit")
    --    kind("SharedLib")
    --    language("C++")
    --    links {
    --        "pthread",
    --        --"boost_thread",
    --        "boost_system",
    --        "boost_regex",
    --        "boost_filesystem",
    --        "jsoncpp"
    --        --"bson",
    --        --"natpmp"
    --    }
    --    files {
    --        --"tests/**.cpp",
    --        --"tests/**.h",
    --        "include/kit/**.cpp",
    --        "include/kit/**.h"
    --    }

    --    --excludes {
    --    --}
    --    --targetsuffix "_test"
    --    --defines {"TESTS"}
    --    defines { "BACKWARD_HAS_BFD=1" }

    --    includedirs {
    --        "../vendor/include/",
    --        "/usr/include/cpp-netlib/"
    --    }

    --    configuration {"debug"}
    --        links {
    --            "bfd",
    --            "iberty"
    --        }
    --        linkoptions { "`llvm-config --libs core` `llvm-config --ldflags`" }
    --    configuration {}

    --    --excludes {
    --    --    "src/tests/*"
    --    --}
    --    configuration { "gmake" }
    --        buildoptions { "-std=c++11",  "-pedantic", "-Wall", "-Wextra" }
    --        configuration { "macosx" }
    --            buildoptions { "-U__STRICT_ANSI__", "-stdlib=libc++" }
    --            linkoptions { "-stdlib=libc++" }
    --    configuration {}

    project("test")
        kind("ConsoleApp")
        language("C++")
        links {
            "pthread",
            --"boost_thread",
            "SDL2",
            "SDL2main",
            "boost_system",
            "boost_regex",
            "boost_filesystem",
            "jsoncpp"
            --"bson",
            --"natpmp"
        }
        files {
            "tests/**.cpp",
            "tests/**.h",
            "include/kit/**.cpp",
            "include/kit/**.h"
        }

        --excludes {
        --}
        --targetsuffix "_test"
        --defines {"TESTS"}
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

        --excludes {
        --    "src/tests/*"
        --}
        configuration { "gmake" }
            --buildoptions { "-std=c++11",  "-pedantic", "-Wall", "-Wextra" }
            buildoptions { "-std=c++11" }
            configuration { "macosx" }
                buildoptions { "-U__STRICT_ANSI__", "-stdlib=libc++" }
                linkoptions { "-stdlib=libc++" }
        configuration {}

