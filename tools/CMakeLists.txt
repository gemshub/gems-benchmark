# options
message(STATUS "Building tools")
file(COPY "${CMAKE_SOURCE_DIR}/Resources/gems3k" DESTINATION "${CMAKE_BINARY_DIR}/bin")
file(COPY "${CMAKE_SOURCE_DIR}/Resources/dbr_diff.json" DESTINATION "${CMAKE_BINARY_DIR}/bin")

include_directories(${CMAKE_SOURCE_DIR})

find_package(Threads REQUIRED)

set(Tools
       thread_test
       recalc_all
       compare_dirs
       collect_metrics)

foreach (toolName ${Tools})
    add_executable(${toolName} ${toolName}.cpp)
    target_link_libraries(${toolName} PUBLIC difftest::difftest Threads::Threads)
    install(TARGETS ${toolName} DESTINATION bin)
endforeach()

