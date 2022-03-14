cmake_minimum_required(VERSION 3.14)
project(WFG-Generator LANGUAGES CXX)

# 是否编译出python模块,默认打开ON,关闭请使用-DPY_MOD=OFF
option(PY_MOD "build python module" OFF)

# 查找LLVM和Clang库
# 使用CONFIG配置模式
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
find_package(Clang REQUIRED CONFIG)
message(STATUS "Found Clang")

set(CMAKE_CXX_STANDARD 14)
# pybind11需要打开-fPIC选项,且不能使用-fno-exception,-fno-rtti选项
if (DEFINED PY_MOD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-Wall -O3 -fPIC")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-Wall -O3 -fno-exception -fno-rtti")
endif ()

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# 添加到LLVM模块的路径
# CMAKE_MODULE_PATH:以分号分隔的列表,供include(),find_package()使用
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}"${LLVM_CMAKE_DIR}")

# 包含LLVM和Clang的头文件
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})

# 添加LLVM和Clang定义的编译器标志Flag
add_definitions(${LLVM_DEFINITIONS})
add_definitions(${CLANG_DEFINITIONS})

# 查找与LLVM组件对应的库
llvm_map_components_to_libnames(LLVM_LIBS support core irreader)

include_directories(include)

if (PY_MOD)
    add_subdirectory(pybind11)
endif ()
add_subdirectory(src)

# 关于构建LLVM和Clang的CMake项目,参考一下链接:
# https://llvm.org/docs/CMake.html#embedding-llvm-in-your-project
# https://stackoverflow.com/questions/55921707/setting-path-to-clang-library-in-cmake