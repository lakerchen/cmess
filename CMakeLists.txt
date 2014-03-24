set(LLVM_LINK_COMPONENTS support)
set(LLVM_USED_LIBS clangTooling clangBasic clangAST)

add_clang_executable(cmess cmess.cpp)

target_link_libraries(cmess
  clangTooling
  clangBasic
  clangASTMatchers
)
