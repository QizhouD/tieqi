# 警告只加在本项目的目标上，不改第三方库自己的编译选项。
# MSVC 加 /utf-8：源码是 UTF-8，注释有中文；不加 /utf-8 时 MSVC 会按系统代码页读源码。

function(tq_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
        if(TQ_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(TQ_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
