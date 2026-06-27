option(BASE_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

if(BASE_ENABLE_SANITIZERS)
    message(STATUS "Sanitizers enabled (ASan + UBSan)")

    set(SAN_FLAGS
        -fsanitize=address
        -fsanitize=undefined
    )

    add_compile_options(${SAN_FLAGS} -g)
    add_link_options(${SAN_FLAGS})

    # Optional: make runtime errors more readable
    add_compile_options(-fno-omit-frame-pointer)
endif()
