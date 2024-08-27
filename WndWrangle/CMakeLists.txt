# CMakeList.txt : CMake project for WndWrangle, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (WndWrangle "WndWrangle.cpp" )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET WndWrangle PROPERTY CXX_STANDARD 20)
endif()

if(MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set(CMAKE_CXX_FLAGS_DEBUG "/MTd ")
endif()

# TODO: Add tests and install targets if needed.
