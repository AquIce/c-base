add_subdirectory(iterators)

file(GLOB CONTAINER_SOURCES CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

target_sources(base PRIVATE ${CONTAINER_SOURCES})
