# ── mcclock-api: Static Library (HTTP API Server) ──

file(GLOB_RECURSE API_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE API_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(mcclock-api STATIC
    ${API_SOURCES}
    ${API_HEADERS}
)

target_include_directories(mcclock-api PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Fetch cpp-httplib
FetchContent_MakeAvailable(httplib)

target_link_libraries(mcclock-api PUBLIC
    mcclock-core
    httplib::httplib
)

# Fetch nlohmann/json for API serialization
FetchContent_MakeAvailable(json)
target_link_libraries(mcclock-api PUBLIC
    nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(mcclock-api PUBLIC
        ws2_32
    )
endif()
