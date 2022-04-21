file(GLOB LIB_SRC "*.cpp")
add_library(wfdg_gen_lib ${LIB_SRC})
target_link_libraries(wfdg_gen_lib PRIVATE ${LLVM_LIBS} clangTooling)