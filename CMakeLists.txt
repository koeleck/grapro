cmake_minimum_required(VERSION 2.8.0)

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
set(MY_WARNING_FLAGS " -Wall -pedantic -Wextra -Werror -Wno-unused-value -Wno-unused-parameter -Wno-unused-function -Wmaybe-uninitialized -Wuninitialized -Wshadow -Wconversion -Wswitch-default -Winit-self -Wunreachable-code ")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -msse2 -msse3 ${MY_WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fno-omit-frame-pointer")
add_definitions( -I${PROJECT_SOURCE_DIR}/src )

#
# Files
#
file(GLOB_RECURSE SRCS src/*.cpp)
file(GLOB_RECURSE HDRS src/*.(h|hpp))

# Packages
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
find_package(Threads)

add_definitions( -DGLEW_NO_GLU ) # we don't use GLU
add_definitions( -DGLFW_INCLUDE_NONE ) # we already include the glew headers

find_package(Boost REQUIRED COMPONENTS system filesystem regex program_options)
add_definitions( -DBOOST_FILESYSTEM_NO_DEPRECATED -DBOOST_FILESYSTEM_VERSION=3)

# glm
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem contrib/glm")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -isystem contrib/glm")
add_definitions( -DGLM_FORCE_RADIANS )

#
# Other libs
#
find_library(FREEIMAGE_LIB freeimage)
if("${FREEIMAGE_LIB}" STREQUAL "")
    message(FATAL_ERROR "FreeImage library not found")
else()
    message(STATUS "FreeImage found")
endif()

find_library(ASSIMP_LIB assimp)
if("${ASSIMP_LIB}" STREQUAL "")
    message(FATAL_ERROR "AssImp library not found")
else()
    message(STATUS "AssImp found")
endif()

add_executable(grapro ${SRCS} ${HDRS})

target_link_libraries(grapro ${OPENGL_gl_LIBRARY} ${GLEW_LIBRARIES} ${Boost_LIBRARIES} ${GLFW_LIBRARIES} ${FREEIMAGE_LIB} ${ASSIMP_LIB} ${CMAKE_THREAD_LIBS_INIT})
