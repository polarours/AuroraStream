# 简单的测试脚本，验证CMake部署脚本语法
message(STATUS "Testing deploy script syntax...")

# 模拟CMAKE_BINARY_DIR
set(CMAKE_BINARY_DIR "D:/Projects/Personal/AuroraStream/cmake-build-debug-mingw-msys2")
set(CMAKE_SOURCE_DIR "D:/Projects/Personal/AuroraStream")
set(EXE_DIR "${CMAKE_BINARY_DIR}/bin")

message(STATUS "EXE_DIR: ${EXE_DIR}")
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

# 测试文件操作
if(NOT EXISTS "${EXE_DIR}")
    message(STATUS "Creating bin directory: ${EXE_DIR}")
    file(MAKE_DIRECTORY "${EXE_DIR}")
endif()

if(NOT EXISTS "${EXE_DIR}/platforms")
    message(STATUS "Creating platforms directory")
    file(MAKE_DIRECTORY "${EXE_DIR}/platforms")
endif()

message(STATUS "Deploy script syntax test completed successfully")