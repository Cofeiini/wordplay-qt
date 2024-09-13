cmake_minimum_required(VERSION 3.5)

project(wordplay-qt VERSION 1.0.0.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)

qt_standard_project_setup(
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES fi
)

qt_add_executable(${PROJECT_NAME}
    MANUAL_FINALIZATION
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/argparser.h
    src/wordplay.h
    src/wordplay.cpp
    src/resources.qrc
)

qt_add_translations(
    TARGETS ${PROJECT_NAME}
    TS_FILE_BASE "wordplay"
    TS_FILE_DIR "${CMAKE_SOURCE_DIR}/localization"
    LUPDATE_OPTIONS -no-obsolete
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    APP_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt::Widgets)

set_target_properties(${PROJECT_NAME} PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

qt_finalize_executable(${PROJECT_NAME})
