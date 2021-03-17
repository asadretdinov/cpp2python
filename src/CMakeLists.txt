cmake_minimum_required(VERSION 3.12)
project(cpp2python)

set(LLVM_PATH D:/Tools/LLVM_Lib)
link_directories(${LLVM_PATH}/lib)
include_directories(${LLVM_PATH}/include)

add_definitions(
-D__STDC_LIMIT_MACROS
-D__STDC_CONSTANT_MACROS
)

set(SOURCE_FILES 
  Lines.cpp
  StatementVisitor.cpp
  DeclarationVisitor.cpp
  ExpressionProcessor.cpp
  main.cpp
)
add_executable(cpp2python ${SOURCE_FILES})

target_link_libraries(cpp2python
  libclang
  clangDriver
  clangFrontend
  clangSerialization
  clangDriver
  clangParse
  clangSema
  clangAnalysis
  clangAST
  clangBasic
  clangEdit
  clangLex
  clangTooling
)

target_link_libraries(cpp2python
  LLVMX86AsmParser # MC, MCParser, Support, X86Desc, X86Info
  LLVMBitstreamReader
  LLVMBinaryFormat
  LLVMDemangle
  LLVMX86Desc # MC, Support, X86AsmPrinter, X86Info
  LLVMX86Info # MC, Support, Target
  LLVMFrontendOpenMP
  LLVMipo
  LLVMScalarOpts
  LLVMInstCombine
  LLVMTransformUtils
  LLVMAnalysis
  LLVMTarget
  LLVMOption # Support
  LLVMProfileData
  LLVMMCParser # MC, Support
  LLVMMC # Object, Support
  LLVMObject # BitReader, Core, Support
  LLVMBitReader # Core, Support
  LLVMCore # Support
  LLVMSupport
  LLVMRemarks
)

target_link_libraries(cpp2python
  version.lib
)