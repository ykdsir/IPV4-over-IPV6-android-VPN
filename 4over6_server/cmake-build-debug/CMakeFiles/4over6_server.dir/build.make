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
CMAKE_SOURCE_DIR = /home/ykd/桌面/yanjy/tun版/IPV4-over-IPV6-android-VPN/4over6_server

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ykd/桌面/yanjy/tun版/IPV4-over-IPV6-android-VPN/4over6_server/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/4over6_server.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/4over6_server.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/4over6_server.dir/flags.make

CMakeFiles/4over6_server.dir/main.cpp.o: CMakeFiles/4over6_server.dir/flags.make
CMakeFiles/4over6_server.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ykd/桌面/yanjy/tun版/IPV4-over-IPV6-android-VPN/4over6_server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/4over6_server.dir/main.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/4over6_server.dir/main.cpp.o -c /home/ykd/桌面/yanjy/tun版/IPV4-over-IPV6-android-VPN/4over6_server/main.cpp

CMakeFiles/4over6_server.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/4over6_server.dir/main.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ykd/桌面/yanjy/tun版/IPV4-over-IPV6-android-VPN/4over6_server/main.cpp > CMakeFiles/4over6_server.dir/main.cpp.i

CMakeFiles/4over6_server.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/4over6_server.dir/main.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ykd/桌面/yanjy/tun版/IPV4-over-IPV6-android-VPN/4over6_server/main.cpp -o CMakeFiles/4over6_server.dir/main.cpp.s

CMakeFiles/4over6_server.dir/main.cpp.o.requires:

.PHONY : CMakeFiles/4over6_server.dir/main.cpp.o.requires

CMakeFiles/4over6_server.dir/main.cpp.o.provides: CMakeFiles/4over6_server.dir/main.cpp.o.requires
	$(MAKE) -f CMakeFiles/4over6_server.dir/build.make CMakeFiles/4over6_server.dir/main.cpp.o.provides.build
.PHONY : CMakeFiles/4over6_server.dir/main.cpp.o.provides

CMakeFiles/4over6_server.dir/main.cpp.o.provides.build: CMakeFiles/4over6_server.dir/main.cpp.o


# Object files for target 4over6_server
4over6_server_OBJECTS = \
"CMakeFiles/4over6_server.dir/main.cpp.o"

# External object files for target 4over6_server
4over6_server_EXTERNAL_OBJECTS =

4over6_server: CMakeFiles/4over6_server.dir/main.cpp.o
4over6_server: CMakeFiles/4over6_server.dir/build.make
4over6_server: CMakeFiles/4over6_server.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ykd/桌面/yanjy/tun版/IPV4-over-IPV6-android-VPN/4over6_server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable 4over6_server"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/4over6_server.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/4over6_server.dir/build: 4over6_server

.PHONY : CMakeFiles/4over6_server.dir/build

CMakeFiles/4over6_server.dir/requires: CMakeFiles/4over6_server.dir/main.cpp.o.requires

.PHONY : CMakeFiles/4over6_server.dir/requires

CMakeFiles/4over6_server.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/4over6_server.dir/cmake_clean.cmake
.PHONY : CMakeFiles/4over6_server.dir/clean

CMakeFiles/4over6_server.dir/depend:
	cd /home/ykd/桌面/yanjy/tun版/IPV4-over-IPV6-android-VPN/4over6_server/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ykd/桌面/yanjy/tun版/IPV4-over-IPV6-android-VPN/4over6_server /home/ykd/桌面/yanjy/tun版/IPV4-over-IPV6-android-VPN/4over6_server /home/ykd/桌面/yanjy/tun版/IPV4-over-IPV6-android-VPN/4over6_server/cmake-build-debug /home/ykd/桌面/yanjy/tun版/IPV4-over-IPV6-android-VPN/4over6_server/cmake-build-debug /home/ykd/桌面/yanjy/tun版/IPV4-over-IPV6-android-VPN/4over6_server/cmake-build-debug/CMakeFiles/4over6_server.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/4over6_server.dir/depend

