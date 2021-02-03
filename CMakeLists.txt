cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(PungsisZero)

find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
set(CMAKE_CXX_STANDARD 17)

include_directories(${PungsisZero_SOURCE_DIR}/include ${TORCH_INSTALL_PREFIX}/include ${TORCH_INSTALL_PREFIX}/include/torch/csrc/api/include)
link_directories(${PungsisZero_SOURCE_DIR}/lib)
add_compile_options(-march=haswell)

add_subdirectory(src)
add_subdirectory(test)

enable_testing()

add_test(NAME RandomBot COMMAND random_bot)