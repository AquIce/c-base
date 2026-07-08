add_subdirectory(adapters)

file(GLOB ITERATOR_SOURCES CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

target_sources(base PRIVATE ${ITERATOR_SOURCES})
