workspace "UI-Builder"
    architecture "x64"

    filter "system:windows" -- static libraries are 32 bit 
        architecture "x86"

    filter {}

    configurations { 
        "Debug", 
        "Release", 
    }

    startproject "Appplication"
    
output_dir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Application"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

	targetdir ("bin/" .. output_dir .. "/")
	objdir ("bin-intermediate/" .. output_dir .. "/")

    flags {"MultiProcessorCompile"}

    runpathdirs { 
        "Vendor/SFML-2.6.1/lib",
    }

    defines {
        "VIRTUAL_DISPLAY",
    }

    files {
        "Source/**.c",
        "Source/**.cpp",
        "Source/**.h",
    }

    includedirs {
		"Source",
	}
    externalincludedirs{
        "Vendor/SFML-2.6.1/include"
    }
    libdirs {
        "Vendor/SFML-2.6.1/lib"
    }
    
    filter "system:macosx"
        links {
            "OpenGL.framework",
            "Cocoa.framework",
            "IOKit.framework",
            "CoreVideo.framework",
            "sfml-system",
            "sfml-window",
            "sfml-graphics",
            "sfml-audio",
        }

    filter {"system:windows", "configurations:Debug"}
        defines {
            "SFML_STATIC",
            "_WINDOWS",
        }
    
        links {
            "opengl32",
            "winmm",
            "gdi32",
            "freetype",
            "flac",
            "ogg",
            "openal32",
            "vorbis",
            "vorbisenc",
            "vorbisfile",
            "sfml-system-s-d",
            "sfml-window-s-d",
            "sfml-graphics-s-d",
            "sfml-audio-s-d",
        }   

    filter {"system:windows", "configurations:Release"}
        defines {
            "SFML_STATIC",
            "_WINDOWS",
        }
    
        links {
            "opengl32",
            "winmm",
            "gdi32",
            "freetype",
            "flac",
            "ogg",
            "openal32",
            "vorbis",
            "vorbisenc",
            "vorbisfile",
            "sfml-system-s",
            "sfml-window-s",
            "sfml-graphics-s",
            "sfml-audio-s",
        }   
        
    filter {}
