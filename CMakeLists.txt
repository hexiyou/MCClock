cmake_minimum_required(VERSION 3.25)
project(MCClock VERSION 1.0.0 LANGUAGES CXX C)

# ── C++ Standard ──
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Qt Auto-processing ──
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Output directories ──
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ── Qt6 ──
# Set Qt6_DIR if Qt is installed at C:\Qt
if(NOT DEFINED Qt6_DIR)
    if(EXISTS "C:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6/Qt6Config.cmake")
        set(Qt6_DIR "C:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6")
    endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Multimedia
    Network
)

qt_standard_project_setup()

# ── FetchContent for header-only dependencies ──
include(FetchContent)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.3
    GIT_SHALLOW TRUE
)

# We don't call MakeAvailable here; sub-projects fetch what they need.
# json and httplib are fetched lazily by the targets that need them.

# ── Windows-specific definitions ──
if(WIN32)
    add_compile_definitions(
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# ── Sub-projects ──
add_subdirectory(src/core)
add_subdirectory(src/api)
add_subdirectory(src/gui)
add_subdirectory(src/cli)

# ── Install rules ──
install(TARGETS MCClock MCClock-CLI
    RUNTIME DESTINATION bin
)
