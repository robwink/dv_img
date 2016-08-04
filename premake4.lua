-- A solution contains projects, and defines the available configurations
solution "DV img"
	configurations { "Debug", "Release" }
	--location "build"
	includedirs { "inc", "src/glcommon" }
	links { "SDL2", "m" }

	configuration "Debug"
		defines { "DEBUG" }
		flags { "Symbols" }

	configuration "Release"
		defines { "NDEBUG" }
		defines { "Optimize" }
	

	project "dv_img"
		location "build"
		kind "ConsoleApp"
		language "C"
		files {
			"dv_img.c",
			"c_utils.c",
			"c_utils.h",
			"basic_types.h",
			"cvector.h",
			"stb_image.h"
		}
		targetdir "build"
		
		configuration { "linux", "gmake" }
			buildoptions { "-std=gnu99", "-fno-strict-aliasing", "-Wall" }
