cmake_minimum_required(VERSION 2.8.0)
include(ExternalProject)

project(GraPro)

#
# misc
#
#set(EXTERN_DIR "${PROJECT_SOURCE_DIR}/extern")
set(CMAKE_BUILD_TYPE Debug)


#
# compiler & flags
#

# warnings
set(MY_WARNING_FLAGS " -Wall -pedantic -Wextra -Werror -Wno-unused-value -Wno-unused-parameter -Wno-unused-but-set-parameter -Wno-unused-function -Wuninitialized -Wshadow -Wconversion -Wswitch-default -Winit-self -Wunreachable-code ")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -msse2 -msse3 -fpermissive ${MY_WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fno-omit-frame-pointer")
add_definitions( -I${PROJECT_SOURCE_DIR}/src )

#
# Files
#
file(GLOB_RECURSE CXX_SRCS src/*.cpp)
file(GLOB_RECURSE C_SRCS src/*.c)
file(GLOB_RECURSE HDRS1 src/*.h)
file(GLOB_RECURSE HDRS1 src/*.hpp)
set(SRCS "${C_SRCS};${CXX_SRCS}")
set(HDRS "${HDRS1};${HDRS2}")

# Packages
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads)

# GLFW
ExternalProject_Add(
    glfw
    PREFIX "contrib"
    URL "http://heanet.dl.sourceforge.net/project/glfw/glfw/3.0.4/glfw-3.0.4.zip"
    CMAKE_ARGS -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF -DCMAKE_INSTALL_PREFIX=/glfw
    INSTALL_COMMAND make DESTDIR=${PROJECT_SOURCE_DIR}/contrib install
)
set(GLFW_STATIC_LIB "${PROJECT_SOURCE_DIR}/contrib/glfw/lib/libglfw3.a")
set(GLFW_LIBRARIES "-lX11 -lXrandr -lXi -lXxf86vm")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${PROJECT_SOURCE_DIR}/contrib/glfw/include")
add_definitions( -DGLFW_INCLUDE_NONE ) # we already include the glew headers

# Glew
add_definitions( -DGLEW_NO_GLU -DGLEW_STATIC )

find_package(Boost REQUIRED COMPONENTS system filesystem regex program_options)
add_definitions( -DBOOST_FILESYSTEM_NO_DEPRECATED -DBOOST_FILESYSTEM_VERSION=3 )

# glm
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${PROJECT_SOURCE_DIR}/contrib/glm")
add_definitions( -DGLM_FORCE_RADIANS )

# stb_image
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${PROJECT_SOURCE_DIR}/contrib/imgui")

#
# Other libs
#
find_library(FREEIMAGE_LIB freeimage)
if("${FREEIMAGE_LIB}" STREQUAL "")
    message(FATAL_ERROR "FreeImage library not found")
elseif("${FREEIMAGE_LIB}" STREQUAL "FREEIMAGE_LIB-NOTFOUND")
	message("FreeImage library not found -> trying to add own")
	include_directories(${PROJECT_SOURCE_DIR}/contrib/FreeImage/Dist)
else()
    message(STATUS "FreeImage found")
endif()

find_library(ASSIMP_LIB assimp)
if("${ASSIMP_LIB}" STREQUAL "")
    message(FATAL_ERROR "AssImp library not found")
else()
    message(STATUS "AssImp found: ${ASSIMP_LIB}")
endif()

add_executable(grapro ${SRCS} ${HDRS})

#target_link_libraries(grapro ${OPENGL_gl_LIBRARY} ${GLEW_LIBRARIES} ${Boost_LIBRARIES} ${GLFW_LIBRARIES} ${FREEIMAGE_LIB} ${ASSIMP_LIB} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(grapro ${GLFW_STATIC_LIB} ${OPENGL_gl_LIBRARY} ${Boost_LIBRARIES} ${GLFW_LIBRARIES} -lfreeimage ${ASSIMP_LIB} ${CMAKE_THREAD_LIBS_INIT})
