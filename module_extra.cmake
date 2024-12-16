# default httplib directory
find_path(HTTPLIB_DIR
        REQUIRED
        NO_CMAKE_FIND_ROOT_PATH
        NAMES
        httplib.h
        HINTS
        ${NAP_ROOT}/modules/naprest/cpp-httplib-0.18.3
)

message(STATUS "httplib dir: ${HTTPLIB_DIR}")

target_include_directories(${PROJECT_NAME} PUBLIC ${HTTPLIB_DIR})