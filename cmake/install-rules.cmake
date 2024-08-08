if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/prism-${PROJECT_VERSION}"
      CACHE STRING ""
  )
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package prism)

install(
    DIRECTORY
    include/
    "${PROJECT_BINARY_DIR}/export/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT prism_Development
)

install(
    TARGETS prism_prism
    EXPORT prismTargets
    RUNTIME #
    COMPONENT prism_Runtime
    LIBRARY #
    COMPONENT prism_Runtime
    NAMELINK_COMPONENT prism_Development
    ARCHIVE #
    COMPONENT prism_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)


# Install the external .lib files
install(
    FILES "${PROJECT_SOURCE_DIR}/lib/vmm.lib" "${PROJECT_SOURCE_DIR}/lib/leechcore.lib"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT prism_Development
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    prism_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE STRING "CMake package config location relative to the install prefix"
)
set_property(CACHE prism_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
mark_as_advanced(prism_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${prism_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT prism_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${prism_INSTALL_CMAKEDIR}"
    COMPONENT prism_Development
)

install(
    EXPORT prismTargets
    NAMESPACE prism::
    DESTINATION "${prism_INSTALL_CMAKEDIR}"
    COMPONENT prism_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
