
file(GLOB ygorclustering_headers "YgorCluster*hpp")

add_library(ygorclustering
    INTERFACE
)    

target_include_directories(ygorclustering 
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS ygorclustering
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${ygorclustering_headers}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

