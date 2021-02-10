cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(PungsisZero)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Torch REQUIRED)
find_package(GSL REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
set(CMAKE_CXX_STANDARD 17)

include_directories(${PungsisZero_SOURCE_DIR}/include ${TORCH_INSTALL_PREFIX}/include ${TORCH_INSTALL_PREFIX}/include/torch/csrc/api/include ${GSL_INCLUDE_DIRS})
link_directories(${PungsisZero_SOURCE_DIR}/lib)

add_compile_options(-march=haswell -O3)

add_subdirectory(src)
add_subdirectory(test)

enable_testing()

add_test(NAME RandomBot COMMAND random_bot)