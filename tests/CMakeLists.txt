add_executable(diag_tests
    main.cpp
    buffer_test.cpp
    codec_test.cpp
    uds_test.cpp
)

target_link_libraries(diag_tests PRIVATE diag)

target_include_directories(diag_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)
