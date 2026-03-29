
file(GLOB ygorclustering_headers "YgorCluster*hpp")

add_library(ygorclustering
    INTERFACE
)    

target_include_directories(ygorclustering 
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
              $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(ygorclustering
    INTERFACE Threads::Threads
)

install(TARGETS ygorclustering
        EXPORT YgorClusteringTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${ygorclustering_headers}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

