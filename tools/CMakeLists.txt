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
       collect_metrics
       # The CI lane's recorder AND its gate: ci_baseline --check re-measures the
       # corpus and scores it against the frozen record in tests/ci-baseline.txt,
       # exiting non-zero on any finding. Native (AIA/SIA) only by design - see
       # include/difftest/ci_baseline.h. tools/ci.sh drives it.
       ci_baseline)

foreach (toolName ${Tools})
    add_executable(${toolName} ${toolName}.cpp)
    target_link_libraries(${toolName} PUBLIC difftest::difftest Threads::Threads)
    install(TARGETS ${toolName} DESTINATION bin)
endforeach()

