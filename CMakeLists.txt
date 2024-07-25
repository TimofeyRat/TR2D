cmake_minimum_required(VERSION 3.5.0)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

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
	Engine/cParticles.cpp
	Engine/cCutscene.cpp
	Engine/cScript.cpp)
	
project(TR2D VERSION 2.0 LANGUAGES CXX)
add_executable(TR2D TR2D.cpp ${TR2D_SOURCES})

target_link_libraries(TR2D sfml-window sfml-graphics sfml-system sfml-audio box2d lua pugixml)

target_compile_options(TR2D PUBLIC -w)

project(MapEditor VERSION 0.1 LANGUAGES CXX)
add_executable(MapEditor TRME.cpp
	Engine/cAnimation.cpp
	Engine/cAssets.cpp
	Engine/cGlobal.cpp
	Engine/cInput.cpp
	Engine/cProgrammable.cpp
	Engine/cWindow.cpp)

target_link_libraries(MapEditor sfml-window sfml-graphics sfml-system sfml-audio pugixml)
target_compile_options(MapEditor PUBLIC -w -static -D SFML_STATIC)

project(SkeletonEditor VERSION 0.1 LANGUAGES CXX)
add_executable(SkeletonEditor TRSE.cpp Engine/cGlobal.cpp)
target_link_libraries(SkeletonEditor  sfml-window sfml-graphics sfml-system sfml-audio pugixml)
target_compile_options(SkeletonEditor PUBLIC -w -static -D SFML_STATIC)