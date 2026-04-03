function(submodule_init)
    foreach (path IN LISTS ARGV)
        message(STATUS "Updating submodule: ${path}")

        execute_process(
            COMMAND git submodule update --init ${path}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE result
            ERROR_VARIABLE error
        )

        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Failed to update submodule ${path}:\n${error}")
        endif()

        execute_process(
            COMMAND git config submodule.${path}.ignore untracked
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endforeach()
endfunction()
