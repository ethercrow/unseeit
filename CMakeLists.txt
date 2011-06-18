cmake_minimum_required(VERSION 2.8)

project(unseeit)

find_package(Qt4 REQUIRED)

file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.h)

qt4_wrap_cpp(HEADERS_MOC ${HEADERS})

include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

set(CMAKE_CXX_FLAGS -std=c++0x) 
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)

add_executable(unseeit ${SOURCES} ${HEADERS_MOC})
target_link_libraries(unseeit ${QT_LIBRARIES})
