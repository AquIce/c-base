file(GLOB MEMORY_EXAMPLES CONFIGURE_DEPENDS
    "*.c"
)

foreach(example_file ${MEMORY_EXAMPLES})

    # strip extension → test name
    get_filename_component(example_name ${example_file} NAME_WE)

    add_executable(${example_name}
        ${example_file}
    )

    target_link_libraries(${example_name}
        PRIVATE base::base
    )

endforeach()
