add_executable(ecu_sim ecu_sim/main.cpp)
target_link_libraries(ecu_sim PRIVATE diag)

add_executable(tester tester/main.cpp)
target_link_libraries(tester PRIVATE diag)
