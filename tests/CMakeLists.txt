# Tests Configuration

# 启用测试
enable_testing()

# 查找Google Test
find_package(GTest REQUIRED)

# 添加单元测试目录
add_subdirectory(unit)

# 添加测试可执行文件
function(add_unit_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name}
            PRIVATE
            GTest::gtest_main
            CoreModule
            MediaModule
            UIModule
    )
    target_include_directories(${test_name} PRIVATE
            ${ROOT_DIR}/include
            ${Qt6Core_INCLUDE_DIRS}
            ${Qt6Widgets_INCLUDE_DIRS}
            ${FFMPEG_INCLUDE_DIRS}
            ${SDL2_INCLUDE_DIRS}
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()