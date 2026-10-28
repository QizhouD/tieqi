# Warnings apply only to this project's targets, not to third-party compile options.
# MSVC /utf-8: sources are UTF-8. Without it MSVC reads the file in the system code page.

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
