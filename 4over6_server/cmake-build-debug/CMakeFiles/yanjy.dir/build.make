# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.6

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


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
CMAKE_COMMAND = /home/ykd/clion-2016.2.2/bin/cmake/bin/cmake

# The command to remove a file.
RM = /home/ykd/clion-2016.2.2/bin/cmake/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ykd/桌面/yanjy/IPV4-over-IPV6-android-VPN/4over6_server

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ykd/桌面/yanjy/IPV4-over-IPV6-android-VPN/4over6_server/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/yanjy.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/yanjy.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/yanjy.dir/flags.make

CMakeFiles/yanjy.dir/main.cpp.o: CMakeFiles/yanjy.dir/flags.make
CMakeFiles/yanjy.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ykd/桌面/yanjy/IPV4-over-IPV6-android-VPN/4over6_server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/yanjy.dir/main.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/yanjy.dir/main.cpp.o -c /home/ykd/桌面/yanjy/IPV4-over-IPV6-android-VPN/4over6_server/main.cpp

CMakeFiles/yanjy.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/yanjy.dir/main.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ykd/桌面/yanjy/IPV4-over-IPV6-android-VPN/4over6_server/main.cpp > CMakeFiles/yanjy.dir/main.cpp.i

CMakeFiles/yanjy.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/yanjy.dir/main.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ykd/桌面/yanjy/IPV4-over-IPV6-android-VPN/4over6_server/main.cpp -o CMakeFiles/yanjy.dir/main.cpp.s

CMakeFiles/yanjy.dir/main.cpp.o.requires:

.PHONY : CMakeFiles/yanjy.dir/main.cpp.o.requires

CMakeFiles/yanjy.dir/main.cpp.o.provides: CMakeFiles/yanjy.dir/main.cpp.o.requires
	$(MAKE) -f CMakeFiles/yanjy.dir/build.make CMakeFiles/yanjy.dir/main.cpp.o.provides.build
.PHONY : CMakeFiles/yanjy.dir/main.cpp.o.provides

CMakeFiles/yanjy.dir/main.cpp.o.provides.build: CMakeFiles/yanjy.dir/main.cpp.o


# Object files for target yanjy
yanjy_OBJECTS = \
"CMakeFiles/yanjy.dir/main.cpp.o"

# External object files for target yanjy
yanjy_EXTERNAL_OBJECTS =

yanjy: CMakeFiles/yanjy.dir/main.cpp.o
yanjy: CMakeFiles/yanjy.dir/build.make
yanjy: CMakeFiles/yanjy.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ykd/桌面/yanjy/IPV4-over-IPV6-android-VPN/4over6_server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable yanjy"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/yanjy.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/yanjy.dir/build: yanjy

.PHONY : CMakeFiles/yanjy.dir/build

CMakeFiles/yanjy.dir/requires: CMakeFiles/yanjy.dir/main.cpp.o.requires

.PHONY : CMakeFiles/yanjy.dir/requires

CMakeFiles/yanjy.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/yanjy.dir/cmake_clean.cmake
.PHONY : CMakeFiles/yanjy.dir/clean

CMakeFiles/yanjy.dir/depend:
	cd /home/ykd/桌面/yanjy/IPV4-over-IPV6-android-VPN/4over6_server/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ykd/桌面/yanjy/IPV4-over-IPV6-android-VPN/4over6_server /home/ykd/桌面/yanjy/IPV4-over-IPV6-android-VPN/4over6_server /home/ykd/桌面/yanjy/IPV4-over-IPV6-android-VPN/4over6_server/cmake-build-debug /home/ykd/桌面/yanjy/IPV4-over-IPV6-android-VPN/4over6_server/cmake-build-debug /home/ykd/桌面/yanjy/IPV4-over-IPV6-android-VPN/4over6_server/cmake-build-debug/CMakeFiles/yanjy.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/yanjy.dir/depend
