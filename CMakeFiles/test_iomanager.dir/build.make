# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Produce verbose output by default.
VERBOSE = 1

# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/adminz/桌面/project/HighPerformanceServer/server

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/adminz/桌面/project/HighPerformanceServer

# Include any dependencies generated for this target.
include CMakeFiles/test_iomanager.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/test_iomanager.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/test_iomanager.dir/flags.make

CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o: CMakeFiles/test_iomanager.dir/flags.make
CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o: server/tests/test_iomanager.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/adminz/桌面/project/HighPerformanceServer/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) -D__FILE__=\"tests/test_iomanager.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o -c /home/adminz/桌面/project/HighPerformanceServer/server/tests/test_iomanager.cpp

CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) -D__FILE__=\"tests/test_iomanager.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/adminz/桌面/project/HighPerformanceServer/server/tests/test_iomanager.cpp > CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.i

CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) -D__FILE__=\"tests/test_iomanager.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/adminz/桌面/project/HighPerformanceServer/server/tests/test_iomanager.cpp -o CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.s

CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o.requires:

.PHONY : CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o.requires

CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o.provides: CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o.requires
	$(MAKE) -f CMakeFiles/test_iomanager.dir/build.make CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o.provides.build
.PHONY : CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o.provides

CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o.provides.build: CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o


# Object files for target test_iomanager
test_iomanager_OBJECTS = \
"CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o"

# External object files for target test_iomanager
test_iomanager_EXTERNAL_OBJECTS =

server/bin/test_iomanager: CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o
server/bin/test_iomanager: CMakeFiles/test_iomanager.dir/build.make
server/bin/test_iomanager: server/lib/libserver.so
server/bin/test_iomanager: /usr/local/lib/libyaml-cpp.a
server/bin/test_iomanager: CMakeFiles/test_iomanager.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/adminz/桌面/project/HighPerformanceServer/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable server/bin/test_iomanager"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_iomanager.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/test_iomanager.dir/build: server/bin/test_iomanager

.PHONY : CMakeFiles/test_iomanager.dir/build

CMakeFiles/test_iomanager.dir/requires: CMakeFiles/test_iomanager.dir/tests/test_iomanager.cpp.o.requires

.PHONY : CMakeFiles/test_iomanager.dir/requires

CMakeFiles/test_iomanager.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/test_iomanager.dir/cmake_clean.cmake
.PHONY : CMakeFiles/test_iomanager.dir/clean

CMakeFiles/test_iomanager.dir/depend:
	cd /home/adminz/桌面/project/HighPerformanceServer && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/adminz/桌面/project/HighPerformanceServer/server /home/adminz/桌面/project/HighPerformanceServer/server /home/adminz/桌面/project/HighPerformanceServer /home/adminz/桌面/project/HighPerformanceServer /home/adminz/桌面/project/HighPerformanceServer/CMakeFiles/test_iomanager.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/test_iomanager.dir/depend

