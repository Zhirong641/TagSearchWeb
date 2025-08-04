cmake_minimum_required(VERSION 3.10)  # 指定最低CMake版本
project(Breakout)      # 定义项目名称和版本
#
# 设置C++标准
# Set compiler options based on the compiler
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2 /std:c++17 /W4 /utf-8")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /DEBUG")
else()
  set(CMAKE_CXX_FLAGS "-std=c++17 -march=native -fopenmp -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
endif()

# find_package(OpenSSL REQUIRED)
# list(APPEND REQUIRED_LIBS ${CMAKE_THREAD_LIBS_INIT})

find_package(Threads REQUIRED)
list(APPEND REQUIRED_LIBS ${OPENSSL_LIBRARIES})

find_package(FlexiMath REQUIRED)
# list(APPEND REQUIRED_LIBS FlexiMath::FlexiMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  list(APPEND REQUIRED_LIBS stdc++fs)
endif()

# 添加可执行文件
add_executable(http_server main.cpp)


target_link_libraries(http_server PRIVATE
  ${REQUIRED_LIBS}
)

# 包含头文件目录
include_directories(${CMAKE_SOURCE_DIR})

# if(MSVC)
#     # Set UTF-8 option for C++ files
#     target_compile_options(client PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/utf-8>)
#     target_compile_options(server PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/utf-8>)
# endif()