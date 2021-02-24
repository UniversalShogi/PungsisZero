include_directories(${PungsisZero_SOURCE_DIR}/src)
add_executable(random_bot random_bot.cpp)
target_link_libraries(random_bot pungsiszero ${GSL_LIBRARIES} ${TORCH_LIBRARIES})