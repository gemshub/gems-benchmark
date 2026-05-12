# options
message(STATUS "Building tools")
file(COPY "${CMAKE_SOURCE_DIR}/Resources/gems3k" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")

include_directories(${CMAKE_SOURCE_DIR})

find_package(Threads REQUIRED)

set(Tools
       thread_test
       recalc_all)

foreach (toolName ${Tools})

    add_executable(${toolName} ${toolName}.cpp)

    if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
        target_link_libraries(${toolName} PUBLIC difftest::difftest GEMS3K::GEMS3K-static Threads::Threads)
    else()
        target_link_libraries(${toolName} PUBLIC difftest::difftest GEMS3K::GEMS3K Threads::Threads)
    endif()

    install(TARGETS ${toolName} DESTINATION bin)
endforeach()

