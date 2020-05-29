function (add_resources outvar)
    if (Qt5QuickCompiler_FOUND)
        if (POLICY CMP0071)
            cmake_policy(SET CMP0071 NEW)
        endif ()

        qtquick_compiler_add_resources(srcs ${ARGN})
        set(${outvar} "${srcs}" PARENT_SCOPE)
    else ()
        set(${outvar} "${ARGN}" PARENT_SCOPE)
    endif()
endfunction ()