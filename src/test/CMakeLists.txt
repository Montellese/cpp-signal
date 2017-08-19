include_directories(".")

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
target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})

add_test(NAME ${PROJECT_NAME}-test
         COMMAND ${PROJECT_NAME}-test --success)