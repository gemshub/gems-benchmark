# The path where cmake config files are installed
set(DIFFTEST_INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/difftest)

install(EXPORT difftestTargets
    FILE difftestTargets.cmake
    NAMESPACE difftest::
    DESTINATION ${DIFFTEST_INSTALL_CONFIGDIR}
    COMPONENT cmake)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/difftestConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/difftestConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/difftestConfig.cmake
    INSTALL_DESTINATION ${DIFFTEST_INSTALL_CONFIGDIR}
    PATH_VARS DIFFTEST_INSTALL_CONFIGDIR)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/difftestConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/difftestConfigVersion.cmake
    DESTINATION ${DIFFTEST_INSTALL_CONFIGDIR})
