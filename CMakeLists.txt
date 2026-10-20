cmake_minimum_required(VERSION 3.21)
project(SpatialWebBrowser VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)
include(CTest)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.10.3 EXACT REQUIRED COMPONENTS Widgets WebEngineWidgets PrintSupport Network)

set(browser_headers
    BrowserWidget BrowserTabBar BrowserRuntime BrowserPreferences BrowserLinkRouter
    BrowserSettingsDialog BrowserExtensionPackage BrowserExtensionCrx BrowserExtensionStore
    BrowserDownloads BrowserAnnotationDialog BrowserExtensionsDialog BrowserHostServices)
set(browser_sources
    BrowserWidget.cpp BrowserWidgetTools.cpp BrowserWidgetPermissions.cpp BrowserTabBar.cpp
    BrowserRuntime.cpp BrowserPreferences.cpp BrowserLinkRouter.cpp BrowserSettingsDialog.cpp
    BrowserExtensionPackage.cpp BrowserExtensionCrx.cpp BrowserExtensionStore.cpp
    BrowserDownloads.cpp BrowserAnnotationDialog.cpp BrowserExtensionsDialog.cpp BrowserHostServices.cpp)
list(TRANSFORM browser_sources PREPEND browser/src/)
set(public_headers)
foreach(header IN LISTS browser_headers)
    list(APPEND public_headers "browser/include/SpatialWeb/Browser/${header}.h")
endforeach()
add_library(SpatialWebBrowser SHARED ${browser_sources} ${public_headers})
add_library(SpatialWeb::Browser ALIAS SpatialWebBrowser)
set_target_properties(SpatialWebBrowser PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 0
    EXPORT_NAME Browser CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
generate_export_header(SpatialWebBrowser BASE_NAME SPATIAL_WEB_BROWSER
    EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/include/SpatialWeb/Browser/SpatialWebBrowserExport.h")
target_include_directories(SpatialWebBrowser PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/browser/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/browser/include/SpatialWeb/Browser>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/SpatialWeb/Browser>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SpatialWeb/Browser>)
target_link_libraries(SpatialWebBrowser PUBLIC Qt6::Widgets Qt6::WebEngineWidgets Qt6::PrintSupport Qt6::Network)
# Private proof input only: consumers/package deployers own the matching Qt runtime.
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/qt-runtime-directory.txt"
    CONTENT "$<TARGET_FILE_DIR:Qt6::Core>")

install(TARGETS SpatialWebBrowser EXPORT SpatialWebBrowserTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${public_headers}
    "${CMAKE_CURRENT_BINARY_DIR}/include/SpatialWeb/Browser/SpatialWebBrowserExport.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SpatialWeb/Browser)
install(EXPORT SpatialWebBrowserTargets NAMESPACE SpatialWeb::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SpatialWebBrowser)
configure_package_config_file(cmake/SpatialWebBrowserConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/SpatialWebBrowserConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SpatialWebBrowser)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/SpatialWebBrowserConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY ExactVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SpatialWebBrowserConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SpatialWebBrowserConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SpatialWebBrowser)
install(FILES LICENSE NOTICE extraction-provenance.json docs/browser-extraction.md docs/composition-boundary.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/SpatialWebBrowser)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    foreach(test IN ITEMS Browser BrowserExtensionCrx BrowserExtensionPackage BrowserExtensionStore BrowserHostServices)
        add_executable(${test}Test tests/${test}Test.cpp)
        target_link_libraries(${test}Test PRIVATE SpatialWeb::Browser Qt6::Test)
        add_test(NAME ${test} COMMAND ${test}Test)
        set_tests_properties(${test} PROPERTIES TIMEOUT 90 ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu")
    endforeach()
endif()

option(SPATIAL_WEB_BUILD_COMPOSITION "Build spatial content discovery and models" ON)
option(SPATIAL_WEB_BUILD_SHELL "Build the ordinary browser application" ON)
if(SPATIAL_WEB_BUILD_SHELL)
    add_executable(spatial-web app/main.cpp)
    target_link_libraries(spatial-web PRIVATE SpatialWeb::Browser)
    install(TARGETS spatial-web RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(SPATIAL_WEB_BUILD_COMPOSITION)
    find_package(SpatialPipe 0.1.0 EXACT CONFIG REQUIRED)
    file(GLOB composition_sources CONFIGURE_DEPENDS composition/src/*.cpp)
    file(GLOB composition_headers CONFIGURE_DEPENDS composition/include/SpatialWeb/Composition/*.h)
    add_library(SpatialWebComposition STATIC ${composition_sources} ${composition_headers} composition/spatialwebmedia.qrc)
    add_library(SpatialWeb::Composition ALIAS SpatialWebComposition)
    set_target_properties(SpatialWebComposition PROPERTIES EXPORT_NAME Composition
        POSITION_INDEPENDENT_CODE ON AUTORCC ON)
    target_compile_definitions(SpatialWebComposition PRIVATE DOCKPIPE_SPATIAL_WEB)
    target_include_directories(SpatialWebComposition PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/composition/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/composition/include/SpatialWeb/Composition>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SpatialWeb/Composition>)
    target_link_libraries(SpatialWebComposition PUBLIC SpatialWeb::Browser SpatialPipe::Runtime)
    install(TARGETS SpatialWebComposition EXPORT SpatialWebCompositionTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES ${composition_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SpatialWeb/Composition)
    install(EXPORT SpatialWebCompositionTargets NAMESPACE SpatialWeb::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SpatialWebComposition)
    configure_package_config_file(cmake/SpatialWebCompositionConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/SpatialWebCompositionConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SpatialWebComposition)
    write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/SpatialWebCompositionConfigVersion.cmake"
        VERSION ${PROJECT_VERSION} COMPATIBILITY ExactVersion)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SpatialWebCompositionConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/SpatialWebCompositionConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SpatialWebComposition)
    if(BUILD_TESTING)
        add_executable(SpatialSiteModelTest tests/SpatialSiteModelTest.cpp)
        target_link_libraries(SpatialSiteModelTest PRIVATE SpatialWeb::Composition)
        add_test(NAME SpatialSiteModel COMMAND SpatialSiteModelTest)
        set_tests_properties(SpatialSiteModel PROPERTIES TIMEOUT 90 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
        add_executable(SpatialCompositionTest tests/SpatialCompositionTest.cpp)
        target_link_libraries(SpatialCompositionTest PRIVATE SpatialWeb::Composition Qt6::Test)
        add_test(NAME SpatialComposition COMMAND SpatialCompositionTest)
        set_tests_properties(SpatialComposition PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()
endif()

option(SPATIAL_WEB_BUILD_PROJECTION "Build spatial browser projection and UI" ${SPATIAL_WEB_BUILD_COMPOSITION})
if(SPATIAL_WEB_BUILD_PROJECTION)
    if(NOT SPATIAL_WEB_BUILD_COMPOSITION)
        message(FATAL_ERROR "Spatial browser projection requires Composition")
    endif()
    find_package(SpatialPipeScene 0.1.0 EXACT CONFIG REQUIRED)
    file(GLOB projection_sources CONFIGURE_DEPENDS projection/src/*.cpp)
    file(GLOB projection_headers CONFIGURE_DEPENDS projection/include/SpatialWeb/Projection/*.h)
    add_library(SpatialWebProjection STATIC ${projection_sources} ${projection_headers}
        projection/spatialwebprojection.qrc)
    add_library(SpatialWeb::Projection ALIAS SpatialWebProjection)
    set_target_properties(SpatialWebProjection PROPERTIES EXPORT_NAME Projection
        POSITION_INDEPENDENT_CODE ON AUTORCC ON)
    target_compile_definitions(SpatialWebProjection PRIVATE DOCKPIPE_SPATIAL_WEB)
    target_include_directories(SpatialWebProjection PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/projection/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/projection/include/SpatialWeb/Projection>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SpatialWeb/Projection>)
    target_link_libraries(SpatialWebProjection PUBLIC SpatialWeb::Composition
        SpatialPipe::Scene SpatialPipe::Geometry)
    install(TARGETS SpatialWebProjection EXPORT SpatialWebProjectionTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES ${projection_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SpatialWeb/Projection)
    install(EXPORT SpatialWebProjectionTargets NAMESPACE SpatialWeb::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SpatialWebProjection)
    configure_package_config_file(cmake/SpatialWebProjectionConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/SpatialWebProjectionConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SpatialWebProjection)
    write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/SpatialWebProjectionConfigVersion.cmake"
        VERSION ${PROJECT_VERSION} COMPATIBILITY ExactVersion)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SpatialWebProjectionConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/SpatialWebProjectionConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SpatialWebProjection)
    if(BUILD_TESTING)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        add_test(NAME SpatialNeuralBoundary COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_neural.py")
        set_tests_properties(SpatialNeuralBoundary PROPERTIES TIMEOUT 30)
        add_executable(SpatialBrowserLifecycleTest tests/SpatialBrowserLifecycleTest.cpp)
        target_link_libraries(SpatialBrowserLifecycleTest PRIVATE SpatialWeb::Projection)
        add_test(NAME SpatialBrowserLifecycle COMMAND SpatialBrowserLifecycleTest)
        set_tests_properties(SpatialBrowserLifecycle PROPERTIES TIMEOUT 90 ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu")
        add_executable(SpatialProjectionTest tests/SpatialProjectionTest.cpp)
        target_compile_definitions(SpatialProjectionTest PRIVATE DOCKPIPE_SPATIAL_WEB)
        target_link_libraries(SpatialProjectionTest PRIVATE SpatialWeb::Projection Qt6::Test)
        add_test(NAME SpatialProjection COMMAND SpatialProjectionTest)
        set_tests_properties(SpatialProjection PROPERTIES TIMEOUT 90 ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu")
    endif()
    if(SPATIAL_WEB_BUILD_SHELL)
        target_compile_definitions(spatial-web PRIVATE SPATIAL_WEB_SHELL_PROJECTION)
        target_link_libraries(spatial-web PRIVATE SpatialWeb::Projection)
    endif()
endif()
