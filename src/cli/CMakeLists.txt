# ── MCClock-CLI: Command Line Executable ──

file(GLOB_RECURSE CLI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE CLI_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_executable(MCClock-CLI
    ${CLI_SOURCES}
    ${CLI_HEADERS}
)

target_include_directories(MCClock-CLI PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(MCClock-CLI PRIVATE
    mcclock-core
)

# CLI does NOT need Qt Widgets or GUI modules
# It only uses QtCore for data types and file I/O
set_target_properties(MCClock-CLI PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
