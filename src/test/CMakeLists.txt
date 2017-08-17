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

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUXX)
  # GCC requires to link against pthread to be able to use std::thread
  find_package(Threads REQUIRED)
  target_link_libraries(${PROJECT_NAME}-test ${CMAKE_THREAD_LIBS_INIT})
  message(STATUS "Threading library: ${CMAKE_THREAD_LIBS_INIT}")
endif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUXX)

add_test(NAME ${PROJECT_NAME}-test
         COMMAND ${PROJECT_NAME}-test --success)