cmake_minimum_required(VERSION 3.5.0)
project(raycaptcha VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 11)

add_library(raycaptcha)
add_executable(rcfrontend)
add_executable(rcjson)

target_include_directories(rcfrontend PUBLIC . glfw/include)
target_include_directories(raycaptcha PUBLIC .)

add_subdirectory(glfw EXCLUDE_FROM_ALL)

if(WIN32)
	target_link_libraries(rcfrontend raycaptcha glfw opengl32 glu32)
else()
	find_package(OpenGL REQUIRED)
	target_link_libraries(rcfrontend raycaptcha glfw OpenGL::GL)
endif()

target_link_libraries(rcjson raycaptcha)

target_sources(raycaptcha PRIVATE
	"raycaptcha.c"
	"terminus8x16.c")

target_sources(rcfrontend PRIVATE 
	"rcfrontend.c"
)

target_sources(rcjson PRIVATE
	"rcjson.c"
)
