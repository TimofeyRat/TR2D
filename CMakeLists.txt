cmake_minimum_required(VERSION 3.5.0)
project(TR2D VERSION 0.1.0 LANGUAGES C CXX)

add_executable(TR2D main.cpp
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
	Engine/pugixml.o)

target_link_libraries(TR2D
	sfml-window-s sfml-system-s opengl32 winmm gdi32
	sfml-graphics-s sfml-system-s sfml-window-s opengl32 freetype
	sfml-system-s winmm
	sfml-audio-s sfml-system-s openal32 flac vorbisenc vorbisfile vorbis ogg
	sfml-network-s sfml-system-s ws2_32
	box2d)

target_compile_options(TR2D PUBLIC -w -static -D SFML_STATIC)

add_custom_command(TARGET TR2D PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:TR2D>/res)