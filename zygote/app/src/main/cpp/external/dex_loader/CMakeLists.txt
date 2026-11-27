add_library(dex_loader INTERFACE)
target_include_directories(dex_loader INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(dex_loader INTERFACE jnihelper)
