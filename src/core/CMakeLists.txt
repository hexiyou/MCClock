# ── mcclock-core: Static Library ──
# Data models, DAL, services, utilities, and SQLite3

# Collect source files
file(GLOB_RECURSE CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/models/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dal/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/services/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/sync/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp"
)

file(GLOB_RECURSE CORE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/models/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/dal/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/services/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/sync/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.h"
)

# SQLite3 amalgamation (compiled as C)
set(SQLITE3_DIR "${CMAKE_SOURCE_DIR}/third_party/sqlite3")
set(SQLITE3_SOURCES "")
if(EXISTS "${SQLITE3_DIR}/sqlite3.c")
    set(SQLITE3_SOURCES "${SQLITE3_DIR}/sqlite3.c")
endif()

add_library(mcclock-core STATIC
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    ${SQLITE3_SOURCES}
)

target_include_directories(mcclock-core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

# SQLite3 include path
if(EXISTS "${SQLITE3_DIR}/sqlite3.h")
    target_include_directories(mcclock-core PUBLIC
        "${SQLITE3_DIR}"
    )
    target_compile_definitions(mcclock-core PUBLIC
        SQLITE_THREADSAFE=1
        SQLITE_ENABLE_JSON1
        SQLITE_ENABLE_FTS5
    )
endif()

# Qt6 dependencies
target_link_libraries(mcclock-core PUBLIC
    Qt6::Core
    Qt6::Multimedia
    Qt6::Network
)

# Sound resource directory: development fallback when running without deployment.
# Deployed builds find sounds next to the executable (<exe dir>/sounds).
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/resources/sounds" MCCLOCK_SOUNDS_SRC_DIR)
target_compile_definitions(mcclock-core PRIVATE
    MCCLOCK_SOUNDS_DIR="${MCCLOCK_SOUNDS_SRC_DIR}"
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(mcclock-core PUBLIC
        ws2_32
        advapi32
        shell32
        user32
    )
endif()
