cmake_minimum_required(VERSION 3.5.0)

set(CMAKE_EXE_LINKER_FLAGS "-static")

set(TR2D_SOURCES
	Engine/cAnimation.cpp
	Engine/cAssets.cpp
	Engine/cDialogue.cpp
	Engine/cEntity.cpp
	Engine/cGlobal.cpp
	Engine/cInput.cpp
	Engine/cInventory.cpp
	Engine/cProgrammable.cpp
	Engine/cRigidbody.cpp
	Engine/cUI.cpp
	Engine/cWearable.cpp
	Engine/cWindow.cpp
	Engine/cWorld.cpp
	Engine/cScript.cpp
	Engine/pugixml.o)
	
project(TR2D VERSION 2.0 LANGUAGES CXX)
add_executable(TR2D TR2D.cpp ${TR2D_SOURCES})

target_link_libraries(TR2D
	sfml-window-s sfml-system-s opengl32 winmm gdi32
	sfml-graphics-s sfml-system-s sfml-window-s opengl32 freetype
	sfml-system-s winmm
	sfml-audio-s sfml-system-s openal32 flac vorbisenc vorbisfile vorbis ogg
	box2d)

target_compile_options(TR2D PUBLIC -w -static -D SFML_STATIC)
target_link_options(TR2D PUBLIC -mwindows)

project(MapEditor VERSION 0.1 LANGUAGES CXX)
add_executable(MapEditor TRME.cpp
	Engine/cWindow.cpp
	Engine/cInput.cpp
	Engine/cGlobal.cpp
	Engine/cAssets.cpp
	Engine/cProgrammable.cpp
	Engine/cAnimation.cpp
	Engine/cAssets.cpp
	Engine/pugixml.o)

target_link_libraries(MapEditor
	sfml-window-s sfml-system-s opengl32 winmm gdi32
	sfml-graphics-s sfml-system-s sfml-window-s opengl32 freetype
	sfml-system-s winmm
	sfml-audio-s sfml-system-s openal32 flac vorbisenc vorbisfile vorbis ogg
	box2d)

target_compile_options(MapEditor PUBLIC -w -static -D SFML_STATIC)
target_link_options(MapEditor PUBLIC -mwindows)

project(SkeletonEditor VERSION 0.1 LANGUAGES CXX)
add_executable(SkeletonEditor TRSE.cpp Engine/pugixml.o)

target_link_libraries(SkeletonEditor
	sfml-window-s sfml-system-s opengl32 winmm gdi32
	sfml-graphics-s sfml-system-s sfml-window-s opengl32 freetype
	sfml-system-s winmm)

target_compile_options(SkeletonEditor PUBLIC -w -static -D SFML_STATIC)
target_link_options(SkeletonEditor PUBLIC -mwindows)