workspace("kit")
    targetdir("bin")
    
    configurations {"Debug", "Release"}

        defines { "GLM_FORCE_RADIANS", "GLM_ENABLE_EXPERIMENTAL", "DO_NOT_USE_WMAIN", "NOMINMAX" }
        
        -- Debug Config
        configuration "Debug"
            defines { "DEBUG" }
            symbols "On"
            linkoptions { }

            configuration "linux"
                links {
                    "z",
                    "bfd",
                    "iberty"
                }
        
        -- Release Config
        configuration "Release"
            defines { "NDEBUG" }
            optimize "speed"
            targetname("kit_dist")

        -- gmake Config
        configuration "gmake"
            buildoptions { "-std=c++11" }
            -- buildoptions { "-std=c++11", "-pedantic", "-Wall", "-Wextra", '-v', '-fsyntax-only'}
            links {
                "pthread",
                "SDL2",
                "boost_system",
                "boost_filesystem",
                "boost_coroutine",
                "boost_python",
                "boost_regex",
                "jsoncpp",
            }
            includedirs {
                "/usr/local/include/",
                "/usr/include/bullet/",
                "/usr/include/raknet/DependentExtensions"
            }

            libdirs {
                "/usr/local/lib"
            }
            
            buildoptions {
                "`python2-config --includes`",
                "`pkg-config --cflags cairomm-1.0 pangomm-1.4`"
            }

            linkoptions {
                "`python2-config --libs`",
                "`pkg-config --libs cairomm-1.0 pangomm-1.4`"
            }
            
        configuration "macosx"
            links {
                "boost_thread-mt",
            }
            
            --buildoptions { "-U__STRICT_ANSI__", "-stdlib=libc++" }
            --linkoptions { "-stdlib=libc++" }

        configuration "linux"
            links {
                --"GL",
                "boost_thread",
            }

        configuration "windows"
            toolset "v140"
            flags { "MultiProcessorCompile" }
        
            links {
                "ws2_32",
                "SDL2",
                "boost_system-vc140-mt-1_61",
                "boost_thread-vc140-mt-1_61",
                "boost_python-vc140-mt-1_61",
                "boost_coroutine-vc140-mt-1_61",
                "boost_regex-vc140-mt-1_61",
                "lib_json",
            }

            includedirs {
                "c:/local/boost_1_61_0",
                "c:/msvc/include",
            }
            configuration { "windows", "Debug" }
                libdirs {
                    "c:/msvc/lib32/debug"
                }
            configuration { "windows" }
            libdirs {
                "c:/msvc/lib32",
                "c:/local/boost_1_61_0/lib32-msvc-14.0",
            }
            -- buildoptions {
                -- "/MP",
                -- "/Gm-",
            -- }
            
            configuration { "windows", "Debug" }
                links {
                    "libboost_filesystem-vc140-mt-gd-1_61",
                }
            configuration {}
            configuration { "windows", "Release" }
                links {
                    "libboost_filesystem-vc140-mt-1_61",
                }

    project "kit"
        kind "ConsoleApp"
        language "C++"

        -- Project Files
        files {
            "tests/**",
            "kit/**"
        }

        -- Exluding Files
        excludes {
            
        }
        
        includedirs {
            "/usr/local/include/",
            "/usr/include/bullet/",
            "/usr/include/raknet/DependentExtensions"
        }

