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

# Bundle sound resources next to the executable so alarm/chime audio works
# both in build tree and in deployment (see tools/deploy.ps1)
if(EXISTS "${CMAKE_SOURCE_DIR}/resources/sounds")
    add_custom_command(TARGET MCClock POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources/sounds"
            "$<TARGET_FILE_DIR:MCClock>/sounds"
        COMMENT "Copying sound resources to output directory"
    )
endif()

# Bundle Qt Chinese translation for standard dialogs (QFontDialog, etc.)
set(QT_TRANS_DIR "C:/Qt/6.8.3/msvc2022_64/translations")
if(EXISTS "${QT_TRANS_DIR}/qtbase_zh_CN.qm")
    add_custom_command(TARGET MCClock POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:MCClock>/translations"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_TRANS_DIR}/qtbase_zh_CN.qm"
            "$<TARGET_FILE_DIR:MCClock>/translations/qtbase_zh_CN.qm"
        COMMENT "Copying Qt Chinese translation to output directory"
    )
endif()
