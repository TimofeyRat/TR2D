# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.30

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = "C:\Program Files\CMake\bin\cmake.exe"

# The command to remove a file.
RM = "C:\Program Files\CMake\bin\cmake.exe" -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = C:\Projects\TR2D

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = C:\Projects\TR2D\build

# Include any dependencies generated for this target.
include CMakeFiles/SkeletonEditor.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/SkeletonEditor.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/SkeletonEditor.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/SkeletonEditor.dir/flags.make

CMakeFiles/SkeletonEditor.dir/TRSE.cpp.obj: CMakeFiles/SkeletonEditor.dir/flags.make
CMakeFiles/SkeletonEditor.dir/TRSE.cpp.obj: C:/Projects/TR2D/TRSE.cpp
CMakeFiles/SkeletonEditor.dir/TRSE.cpp.obj: CMakeFiles/SkeletonEditor.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=C:\Projects\TR2D\build\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/SkeletonEditor.dir/TRSE.cpp.obj"
	C:\PROGRA~1\mingw64\bin\G__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/SkeletonEditor.dir/TRSE.cpp.obj -MF CMakeFiles\SkeletonEditor.dir\TRSE.cpp.obj.d -o CMakeFiles\SkeletonEditor.dir\TRSE.cpp.obj -c C:\Projects\TR2D\TRSE.cpp

CMakeFiles/SkeletonEditor.dir/TRSE.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/SkeletonEditor.dir/TRSE.cpp.i"
	C:\PROGRA~1\mingw64\bin\G__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E C:\Projects\TR2D\TRSE.cpp > CMakeFiles\SkeletonEditor.dir\TRSE.cpp.i

CMakeFiles/SkeletonEditor.dir/TRSE.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/SkeletonEditor.dir/TRSE.cpp.s"
	C:\PROGRA~1\mingw64\bin\G__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S C:\Projects\TR2D\TRSE.cpp -o CMakeFiles\SkeletonEditor.dir\TRSE.cpp.s

CMakeFiles/SkeletonEditor.dir/Engine/cGlobal.cpp.obj: CMakeFiles/SkeletonEditor.dir/flags.make
CMakeFiles/SkeletonEditor.dir/Engine/cGlobal.cpp.obj: C:/Projects/TR2D/Engine/cGlobal.cpp
CMakeFiles/SkeletonEditor.dir/Engine/cGlobal.cpp.obj: CMakeFiles/SkeletonEditor.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=C:\Projects\TR2D\build\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/SkeletonEditor.dir/Engine/cGlobal.cpp.obj"
	C:\PROGRA~1\mingw64\bin\G__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/SkeletonEditor.dir/Engine/cGlobal.cpp.obj -MF CMakeFiles\SkeletonEditor.dir\Engine\cGlobal.cpp.obj.d -o CMakeFiles\SkeletonEditor.dir\Engine\cGlobal.cpp.obj -c C:\Projects\TR2D\Engine\cGlobal.cpp

CMakeFiles/SkeletonEditor.dir/Engine/cGlobal.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/SkeletonEditor.dir/Engine/cGlobal.cpp.i"
	C:\PROGRA~1\mingw64\bin\G__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E C:\Projects\TR2D\Engine\cGlobal.cpp > CMakeFiles\SkeletonEditor.dir\Engine\cGlobal.cpp.i

CMakeFiles/SkeletonEditor.dir/Engine/cGlobal.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/SkeletonEditor.dir/Engine/cGlobal.cpp.s"
	C:\PROGRA~1\mingw64\bin\G__~1.EXE $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S C:\Projects\TR2D\Engine\cGlobal.cpp -o CMakeFiles\SkeletonEditor.dir\Engine\cGlobal.cpp.s

# Object files for target SkeletonEditor
SkeletonEditor_OBJECTS = \
"CMakeFiles/SkeletonEditor.dir/TRSE.cpp.obj" \
"CMakeFiles/SkeletonEditor.dir/Engine/cGlobal.cpp.obj"

# External object files for target SkeletonEditor
SkeletonEditor_EXTERNAL_OBJECTS =

C:/Projects/TR2D/bin/SkeletonEditor.exe: CMakeFiles/SkeletonEditor.dir/TRSE.cpp.obj
C:/Projects/TR2D/bin/SkeletonEditor.exe: CMakeFiles/SkeletonEditor.dir/Engine/cGlobal.cpp.obj
C:/Projects/TR2D/bin/SkeletonEditor.exe: CMakeFiles/SkeletonEditor.dir/build.make
C:/Projects/TR2D/bin/SkeletonEditor.exe: CMakeFiles/SkeletonEditor.dir/linkLibs.rsp
C:/Projects/TR2D/bin/SkeletonEditor.exe: CMakeFiles/SkeletonEditor.dir/objects1.rsp
C:/Projects/TR2D/bin/SkeletonEditor.exe: CMakeFiles/SkeletonEditor.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=C:\Projects\TR2D\build\CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable C:\Projects\TR2D\bin\SkeletonEditor.exe"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\SkeletonEditor.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/SkeletonEditor.dir/build: C:/Projects/TR2D/bin/SkeletonEditor.exe
.PHONY : CMakeFiles/SkeletonEditor.dir/build

CMakeFiles/SkeletonEditor.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\SkeletonEditor.dir\cmake_clean.cmake
.PHONY : CMakeFiles/SkeletonEditor.dir/clean

CMakeFiles/SkeletonEditor.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" C:\Projects\TR2D C:\Projects\TR2D C:\Projects\TR2D\build C:\Projects\TR2D\build C:\Projects\TR2D\build\CMakeFiles\SkeletonEditor.dir\DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/SkeletonEditor.dir/depend

