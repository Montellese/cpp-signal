CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

set(PROJECT_NAME cpp-signal)
project(${PROJECT_NAME})

set(${PROJECT_NAME}_MAJOR_VERSION 0)
set(${PROJECT_NAME}_MINOR_VERSION 1)
set(${PROJECT_NAME}_PATCH_VERSION 0)
set(${PROJECT_NAME}_VERSION
    ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})

# enable C++11 without GNU extensions
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

# detect the compiler being used for special handling
set(IS_GCC false)
set(IS_CLANG false)
set(IS_MSVC false)
string(TOLOWER "${CMAKE_CXX_COMPILER_ID}" CXX_COMPILER_ID)
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUXX)
  set(IS_GCC true)
elseif(CXX_COMPILER_ID MATCHES "clang")
  set(IS_CLANG true)
elseif(MSVC)
  set(IS_MSVC true)
endif()

set(INCLUDE_DIR "include")

set(HEADERS ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/${PROJECT_NAME}.h
            ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/${PROJECT_NAME}-async.h
            ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/${PROJECT_NAME}-locking.h)

# header-only library
add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${HEADERS}>)
target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}/>)
target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)

# GCC requires to link against pthread to be able to use std::thread
if(IS_GCC OR IS_CLANG)
  find_package(Threads REQUIRED)
  target_link_libraries(${PROJECT_NAME} INTERFACE ${CMAKE_THREAD_LIBS_INIT})
  message(STATUS "Threading library: ${CMAKE_THREAD_LIBS_INIT}")
endif()

# add flags for more strict error checking / reporting
if(IS_GCC OR IS_CLANG)
  add_definitions(-Wall)
  add_definitions(-Wextra)
  add_definitions(-Werror)
  add_definitions(-pedantic)
elseif(IS_MSVC)
  add_definitions(/W2)
endif()

# generate package config
include(CMakePackageConfigHelpers)
set(CONFIG_PACKAGE_INSTALL_DIR lib/cmake/${PROJECT_NAME})
set(CONFIG_PACKAGE_VERSION ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake)
set(CONFIG_PACKAGE_CONFIG ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-targets)

write_basic_package_version_file(${CONFIG_PACKAGE_VERSION}
                                 VERSION ${${PROJECT_NAME}_VERSION}
                                 COMPATIBILITY SameMajorVersion)

configure_package_config_file(${PROJECT_SOURCE_DIR}/${PROJECT_NAME}-config.cmake.in
                              ${CONFIG_PACKAGE_CONFIG}
                              INSTALL_DESTINATION ${CONFIG_PACKAGE_INSTALL_DIR})
export(TARGETS ${PROJECT_NAME} NAMESPACE ${PROJECT_NAME}:: FILE ${PROJECT_BINARY_DIR}/${TARGETS_EXPORT_NAME}.cmake)

# install target
install(FILES ${CONFIG_PACKAGE_VERSION} ${CONFIG_PACKAGE_CONFIG} DESTINATION ${CONFIG_PACKAGE_INSTALL_DIR})
install(EXPORT ${TARGETS_EXPORT_NAME} DESTINATION ${CONFIG_PACKAGE_INSTALL_DIR} NAMESPACE ${PROJECT_NAME}::)
install(TARGETS ${PROJECT_NAME} EXPORT ${TARGETS_EXPORT_NAME} DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include)

# testing
option(BUILD_TESTING "build tests" ON)
if(${BUILD_TESTING})
  enable_testing()

  add_subdirectory(src)
endif()
