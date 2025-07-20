cmake_minimum_required(VERSION 3.20)


project(IPS VERSION 1.0.0 LANGUAGES CXX C)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -ffast-math)
endif()


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")


find_package(ZLIB REQUIRED)

set(IPS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/image.cpp
)

set(IPS_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/decoder/png.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/image.hpp
)


add_library(ips STATIC
    ${IPS_SOURCES}
    ${IPS_HEADERS}
)

set_target_properties(ips PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(ips
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ips
    PUBLIC
    ZLIB::ZLIB
    )
    
target_compile_features(ips PUBLIC cxx_std_20)
    
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
    
if(BUILD_SHARED_LIBS)
    add_library(ips_shared SHARED
        ${IPS_SOURCES}
        ${IPS_HEADERS}
    )
    
    set_target_properties(ips_shared PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "${IPS_HEADERS}"
        OUTPUT_NAME ips
    )
    
    target_include_directories(ips_shared
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(ips_shared
        PUBLIC
            ZLIB::ZLIB
    )
    
    target_compile_features(ips_shared PUBLIC cxx_std_20)
endif()