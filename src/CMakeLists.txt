file(GLOB LIB_SRC "*.cpp")
add_executable(test ${LIB_SRC})
target_link_libraries(test PRIVATE ${LLVM_LIBS} clangTooling)