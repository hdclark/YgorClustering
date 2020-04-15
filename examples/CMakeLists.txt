
file(GLOB example_sources "*cc")
file(GLOB compile_sources "*sh")

INSTALL(FILES ${example_sources}
              ${compile_sources}
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/ygorclustering/examples/"
)

