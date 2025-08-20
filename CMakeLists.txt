cmake_minimum_required(VERSION 3.16)

# Project metadata
project(EmuDiscer VERSION 1.1 LANGUAGES CXX)

# --- Global setup ---

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find required Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent LinguistTools)

# Platform-specific dependencies
if(WIN32)
    find_package(Qt6 REQUIRED COMPONENTS WinExtras)
elseif(UNIX AND NOT ANDROID)
    find_package(Qt6 REQUIRED COMPONENTS DBus)
endif()

# --- Executable target ---

# Translation support
set(TS_FILES
    EmuDiscer_en_US.ts
)

qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

# Sources, headers, forms
set(SOURCES
    UDisks2Watcher.cpp
    Utilities.cpp
    appdialog.cpp
    emudiscer.cpp
    main.cpp
)

set(HEADERS
    EmulatorBuiltInOptions.h
    MiIni.h
    UDisks2Watcher.h
    Utilities.h
    appdialog.h
    emudiscer.h
)

set(FORMS
    appdialog.ui
    emudiscer.ui
)

set(RESOURCES
    resources.qrc
)

# Create executable
qt_add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
    ${RESOURCES}
    ${QM_FILES}
)

# Define version macro
target_compile_definitions(${PROJECT_NAME} PRIVATE
    EMUDISCER_APP_VERSION="${PROJECT_VERSION}"
)

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::WinExtras)
elseif(UNIX AND NOT ANDROID)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::DBus)
endif()

# --- Installation ---

# Set application icon on Windows
if(WIN32)
    set(WIN_ICON ${CMAKE_SOURCE_DIR}/EmuDiscer.ico)
    target_sources(${PROJECT_NAME} PRIVATE ${WIN_ICON})
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Installation rules
if(UNIX AND NOT ANDROID)
    install(TARGETS ${PROJECT_NAME} DESTINATION /opt/${PROJECT_NAME}/bin)
elseif(QNX)
    install(TARGETS ${PROJECT_NAME} DESTINATION /tmp/${PROJECT_NAME}/bin)
endif()
