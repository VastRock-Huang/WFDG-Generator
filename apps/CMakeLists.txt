add_executable(wfdg_gen_tool main.cpp)
target_link_libraries(wfdg_gen_tool wfdg_gen_lib)

if (PY_MOD)
    pybind11_add_module(wfdg_generator pybinder.cpp)
    target_link_libraries(wfdg_generator PRIVATE wfdg_gen_lib)
endif ()