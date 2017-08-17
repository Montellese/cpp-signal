include_directories(".")

set(CMAKE_CXX_STANDARD 11)

set(SOURCES main.cpp
            aggregation.cpp
            async.cpp
            class.cpp
            copy.cpp
            global.cpp
            lambda.cpp
            signal.cpp
            signal_chain.cpp
            slot_tracker.cpp)

add_executable(${PROJECT_NAME}-test ${HEADERS} ${SOURCES})

# detect the compiler being used for special handling
set(IS_GCC false)
set(IS_CLANG false)
string(TOLOWER "${CMAKE_CXX_COMPILER_ID}" CXX_COMPILER_ID)
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUXX)
  set(IS_GCC true)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "clang")
  set(IS_CLANG true)
endif()

if(IS_GCC)
  # GCC requires to link against pthread to be able to use std::thread
  find_package(Threads REQUIRED)
  target_link_libraries(${PROJECT_NAME}-test ${CMAKE_THREAD_LIBS_INIT})
  message(STATUS "Threading library: ${CMAKE_THREAD_LIBS_INIT}")
endif(IS_GCC)

if(IS_GCC OR IS_CLANG)
  # add flags for more strict error checking / reporting
  add_definitions(-Wall)
  add_definitions(-Wextra)
  add_definitions(-Werror)
  add_definitions(-pedantic)
endif()

add_test(NAME ${PROJECT_NAME}-test
         COMMAND ${PROJECT_NAME}-test --success)