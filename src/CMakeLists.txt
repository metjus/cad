# Layering (lower layers never include higher ones):
#   core      - units, expressions, errors (no OCCT, no Qt)
#   mesh      - polygon mesh import + analysis (no OCCT, no Qt)
#   kernel    - thin wrappers over OCCT: named shapes, primitives, NURBS
#   document  - parameters, features, history, recompute (no Qt)
#   app       - Qt Widgets UI + OCCT viewer (presentation only)

add_library(hcad_core STATIC
  core/Expression.cpp
  core/Units.cpp
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
  qt_add_executable(hcad_poc WIN32
    app/main.cpp
    app/MainWindow.cpp
    app/MainWindow.h
    app/OcctViewWidget.cpp
    app/OcctViewWidget.h
    app/SceneSync.cpp
    app/SceneSync.h
    app/Theme.cpp
    app/Theme.h
    app/SmokeTest.cpp
    app/SmokeTest.h
  )
  target_link_libraries(hcad_poc PRIVATE hcad_document Qt6::Widgets
    TKService TKV3d TKOpenGl)
  if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(hcad_poc PRIVATE X11::X11)
  endif()
  if(WIN32)
    # Collect the DLLs the executable needs (OCCT, Qt, TBB, FreeType, ...)
    # from the build environment, excluding Windows system libraries.
    install(TARGETS hcad_poc
      RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
        POST_EXCLUDE_REGEXES ".*[Ss]ystem32/.*\\.dll"
        DIRECTORIES ${HCAD_RUNTIME_DIRS}
      RUNTIME DESTINATION .)
  else()
    install(TARGETS hcad_poc RUNTIME DESTINATION bin)
  endif()
endif()
