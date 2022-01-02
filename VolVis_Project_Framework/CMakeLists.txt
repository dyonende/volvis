cmake_minimum_required(VERSION 3.15)
project(GLViewer)

# Slightly modified versions of the files from the cpp-starter-project by Jason Turner:
# https://github.com/lefticus/cpp_starter_project/
include("cmake/CompilerWarnings.cmake") # Enable almost all compiler warnings and CMake option to enable -Werror.

# Download vcpkg and use it to install the dependencies unless vcpkg is already installed.
#if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
#	include("cmake/pmm.cmake")
#	pmm(DEBUG VCPKG
#		REVISION c4937039b0704c711dff11ffa729f1c105b20e42
#		REQUIRES glfw3 glew glm ms-gsl imgui nativefiledialog tbb fmt catch2)
#endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(unofficial-nativefiledialog CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

add_library(VolVis "")
set_project_warnings(VolVis)
include(${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt)
target_include_directories(VolVis PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src/")
target_compile_features(VolVis PUBLIC cxx_std_20)
target_link_libraries(VolVis
	PUBLIC
		glm::glm
		imgui::imgui
		unofficial::nativefiledialog::nfd
		TBB::tbb
		Threads::Threads
		Microsoft.GSL::GSL
		fmt::fmt)

add_executable(Viewer "src/main.cpp")
set_project_warnings(Viewer)
target_link_libraries(Viewer
	PRIVATE
		VolVis
		OpenGL::GL
		glfw
		GLEW::GLEW)

# Copy glsl files to build directory
configure_file("${CMAKE_CURRENT_LIST_DIR}/shaders/viewer_output.vs" "${CMAKE_CURRENT_BINARY_DIR}/viewer_output.vs" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/shaders/viewer_output.fs" "${CMAKE_CURRENT_BINARY_DIR}/viewer_output.fs" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/shaders/wireframe_cube.vs" "${CMAKE_CURRENT_BINARY_DIR}/wireframe_cube.vs" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/shaders/wireframe_cube.fs" "${CMAKE_CURRENT_BINARY_DIR}/wireframe_cube.fs" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/shaders/surface_cube.vs" "${CMAKE_CURRENT_BINARY_DIR}/surface_cube.vs" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/shaders/surface_cube.fs" "${CMAKE_CURRENT_BINARY_DIR}/surface_cube.fs" COPYONLY)

enable_testing()
add_subdirectory("integrity_tests")
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/grading/")
	add_subdirectory("grading")
endif()