file(GLOB ITERATORS_EXAMPLES CONFIGURE_DEPENDS
    "*.c"
)

foreach(example_file ${ITERATORS_EXAMPLES})

    get_filename_component(example_name ${example_file} NAME_WE)

    add_executable(${example_name}
        ${example_file}
    )

    target_link_libraries(${example_name}
        PRIVATE base::base
    )

endforeach()
