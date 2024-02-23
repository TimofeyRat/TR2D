cmake_minimum_required(VERSION 3.5.0)

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
	Engine/pugixml.o)


	
project(TR2D VERSION 2.0 LANGUAGES CXX)
add_executable(TR2D TR2D.cpp ${TR2D_SOURCES})

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



project(MapEditor VERSION 0.1 LANGUAGES CXX)

add_executable(MapEditor mapEditor.cpp ${TR2D_SOURCES})

target_link_libraries(MapEditor
	sfml-window-s sfml-system-s opengl32 winmm gdi32
	sfml-graphics-s sfml-system-s sfml-window-s opengl32 freetype
	sfml-system-s winmm
	sfml-audio-s sfml-system-s openal32 flac vorbisenc vorbisfile vorbis ogg
	sfml-network-s sfml-system-s ws2_32
	box2d)

target_compile_options(MapEditor PUBLIC -w -static -D SFML_STATIC)

add_custom_command(TARGET MapEditor PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:MapEditor>/res)