file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
    "*.c"
)

foreach(test_file ${TEST_SOURCES})

    # strip extension → test name
    get_filename_component(test_name ${test_file} NAME_WE)

    add_executable(${test_name}
        ${test_file}
    )

    target_link_libraries(${test_name}
        PRIVATE base::base
    )

    add_test(NAME ${test_name} COMMAND ${test_name})

endforeach()

