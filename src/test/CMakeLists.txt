include_directories(".")

set(SOURCES main.cpp
            class.cpp
            global.cpp
            lambda.cpp
            signal.cpp
            signal_chain.cpp
            slot_tracker.cpp)

add_executable(${PROJECT_NAME}-test ${HEADERS} ${SOURCES})

add_test(NAME ${PROJECT_NAME}-test
         COMMAND ${PROJECT_NAME}-test --success)