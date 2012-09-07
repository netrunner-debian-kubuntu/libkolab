
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DTEST_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

macro(addTest TEST_NAME)
    QT4_AUTOMOC(${TEST_NAME}.cpp)
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME} ${QT_QTTEST_LIBRARY} kolab_static)
    add_test(${TEST_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME})
endmacro()

QT4_AUTOMOC(benchmark.cpp)
add_executable(benchmarktest benchmark.cpp)
target_link_libraries(benchmarktest ${QT_QTTEST_LIBRARY} kolab_static)

addTest(formattest)
addTest(upgradetest)
addTest(kcalconversiontest)
addTest(calendaringtest)
addTest(icalendartest)
addTest(freebusytest)
addTest(kolabobjecttest)
