# ── MCClock: GUI Executable ──

file(GLOB_RECURSE GUI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE GUI_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Qt Resource files
set(GUI_RESOURCES "")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.qrc")
    set(GUI_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.qrc")
endif()

# Windows resource file (icon, version info)
set(WIN_RC "")
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/mcclock.rc")
    set(WIN_RC "${CMAKE_CURRENT_SOURCE_DIR}/resources/mcclock.rc")
endif()

qt_add_executable(MCClock WIN32
    ${GUI_SOURCES}
    ${GUI_HEADERS}
    ${GUI_RESOURCES}
    ${WIN_RC}
)

target_include_directories(MCClock PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(MCClock PRIVATE
    mcclock-core
    mcclock-api
    Qt6::Widgets
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Network
)

# Set working directory for debugging
set_target_properties(MCClock PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
