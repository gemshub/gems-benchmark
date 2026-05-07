
# Recursively collect all header files from the current directory
file(GLOB_RECURSE HEADER_FILES_TOP  ${CMAKE_SOURCE_DIR}/include/difftest/*.h )

# Recursively collect all header files from the current directory
file(GLOB_RECURSE HEADER_FILES  *.h )

# Recursively collect all source files from the current directory
file(GLOB_RECURSE SOURCE_FILES  *.cpp )

# Enable automatic creation of a module definition (.def) file for a SHARED library on Windows.
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

# Build a difftest C++ library
add_library(difftest ${HEADER_FILES_TOP} ${HEADER_FILES} ${SOURCE_FILES})

# Add an alias to the target library difftest
add_library(difftest::difftest ALIAS difftest)

# Add the include paths to the ReactionsGenerator target
target_include_directories(difftest
    PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR};${PROJECT_SOURCE_DIR}/src;${PROJECT_SOURCE_DIR}/include>"
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Create install target for library
install(TARGETS difftest
    EXPORT difftestTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT libraries)

# Install debug symbols
if(MSVC)
    install(
        FILES $<TARGET_PDB_FILE:difftest>
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT libraries
        OPTIONAL
    )
endif()

# Install the header files preserving the directory hierarchy
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/difftest
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

