# Layering (lower layers never include higher ones):
#   core      - units, expressions, errors (no OCCT, no Qt)
#   mesh      - polygon mesh import + analysis (no OCCT, no Qt)
#   kernel    - thin wrappers over OCCT: named shapes, primitives, NURBS
#   document  - parameters, features, history, recompute (no Qt)
#   app       - Qt Widgets UI + OCCT viewer (presentation only)

add_library(hcad_core STATIC
  core/Expression.cpp
  core/Units.cpp
  core/FuzzyMatch.cpp
)
target_include_directories(hcad_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(hcad_mesh STATIC
  mesh/TriMesh.cpp
  mesh/MeshIO.cpp
  mesh/MeshAnalysis.cpp
)
target_include_directories(hcad_mesh PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(hcad_kernel STATIC
  kernel/NamedShape.cpp
  kernel/Primitives.cpp
  kernel/Nurbs.cpp
  kernel/ShapeInfo.cpp
  kernel/MeshBridge.cpp
)
target_include_directories(hcad_kernel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OpenCASCADE_INCLUDE_DIR})
target_link_libraries(hcad_kernel PUBLIC hcad_core hcad_mesh
  TKernel TKMath TKG3d TKG2d TKBRep TKTopAlgo TKGeomBase TKGeomAlgo TKPrim TKFillet TKBO TKShHealing TKMesh)

add_library(hcad_document STATIC
  document/ParameterTable.cpp
  document/Feature.cpp
  document/Features.cpp
  document/Document.cpp
)
target_include_directories(hcad_document PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hcad_document PUBLIC hcad_kernel)

if(HCAD_BUILD_APP)
  find_package(Qt6 REQUIRED COMPONENTS Widgets)
  set(CMAKE_AUTOMOC ON)
  configure_file(app/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/app/Version.h @ONLY)

  # UI library: everything except main(), so UI tests can link it.
  add_library(hcad_ui STATIC
    app/AppPaths.cpp app/AppPaths.h
    app/CommandPalette.cpp app/CommandPalette.h
    app/CommandRegistry.cpp app/CommandRegistry.h
    app/Icons.cpp app/Icons.h
    app/Logging.cpp app/Logging.h
    app/MainWindow.cpp app/MainWindow.h
    app/OcctViewWidget.cpp app/OcctViewWidget.h
    app/Preferences.cpp app/Preferences.h
    app/PreferencesDialog.cpp app/PreferencesDialog.h
    app/SceneSync.cpp app/SceneSync.h
    app/SmokeTest.cpp app/SmokeTest.h
    app/Theme.cpp app/Theme.h
    app/UpdateDialog.cpp app/UpdateDialog.h
    app/UpdateService.cpp app/UpdateService.h
    app/UpdateTest.cpp app/UpdateTest.h
    app/WelcomeDialog.cpp app/WelcomeDialog.h
  )
  target_include_directories(hcad_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/generated)
  target_link_libraries(hcad_ui PUBLIC hcad_document Qt6::Widgets TKService TKV3d TKOpenGl)
  if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(hcad_ui PUBLIC X11::X11)
  endif()

  if(HCAD_WITH_VELOPACK)
    find_path(VELOPACK_INCLUDE_DIR Velopack.h PATHS ${VELOPACK_DIR}/include NO_DEFAULT_PATH REQUIRED)
    # Prefer the DLL import library (the static Rust library needs a long
    # list of extra system libraries).
    file(GLOB _vpk_implibs "${VELOPACK_DIR}/lib/*win*x64*.dll.lib" "${VELOPACK_DIR}/lib/*x64*.dll.lib")
    file(GLOB _vpk_dlls "${VELOPACK_DIR}/lib/*win*x64*.dll" "${VELOPACK_DIR}/lib/*x64*.dll")
    if(NOT _vpk_implibs OR NOT _vpk_dlls)
      message(FATAL_ERROR "Velopack x64 import library/DLL not found under ${VELOPACK_DIR}/lib")
    endif()
    list(GET _vpk_implibs 0 VELOPACK_IMPLIB)
    list(GET _vpk_dlls 0 _vpk_dll_src)
    # The import library refers to "velopack_libc.dll", but the release ships
    # it with a platform suffix; provide it under the expected name.
    set(VELOPACK_DLL ${CMAKE_BINARY_DIR}/velopack-runtime/velopack_libc.dll)
    configure_file(${_vpk_dll_src} ${VELOPACK_DLL} COPYONLY)
    set(HCAD_VELOPACK_DLL ${VELOPACK_DLL} CACHE INTERNAL "Velopack runtime DLL")
    message(STATUS "Velopack: ${VELOPACK_IMPLIB} / ${VELOPACK_DLL}")
    target_include_directories(hcad_ui PRIVATE ${VELOPACK_INCLUDE_DIR})
    target_compile_definitions(hcad_ui PRIVATE HCAD_WITH_VELOPACK)
    target_link_libraries(hcad_ui PUBLIC ${VELOPACK_IMPLIB})
  endif()

  qt_add_executable(hcad_app WIN32 app/main.cpp)
  set_target_properties(hcad_app PROPERTIES OUTPUT_NAME HybridCAD)
  target_link_libraries(hcad_app PRIVATE hcad_ui)
  if(HCAD_WITH_VELOPACK)
    # The Velopack DLL must sit next to every executable that links it.
    add_custom_command(TARGET hcad_app POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VELOPACK_DLL} $<TARGET_FILE_DIR:hcad_app>)
  endif()

  if(WIN32)
    # Collect the DLLs the executable needs (OCCT, Qt, TBB, FreeType, ...)
    # from the build environment, excluding Windows system libraries.
    set(_dirs ${HCAD_RUNTIME_DIRS})
    if(HCAD_WITH_VELOPACK)
      get_filename_component(_vpkdir ${VELOPACK_DLL} DIRECTORY)
      list(APPEND _dirs ${_vpkdir})
    endif()
    install(TARGETS hcad_app
      RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
        POST_EXCLUDE_REGEXES ".*[Ss]ystem32/.*\\.dll"
        DIRECTORIES ${_dirs}
      RUNTIME DESTINATION .)
  else()
    install(TARGETS hcad_app RUNTIME DESTINATION bin)
  endif()
endif()
