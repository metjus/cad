find_package(GTest REQUIRED)
add_executable(hcad_tests
  test_expression.cpp
  test_box_feature.cpp
  test_topo_naming.cpp
  test_nurbs.cpp
  test_mesh.cpp
  test_fuzzy.cpp
)
target_link_libraries(hcad_tests PRIVATE hcad_document GTest::gtest GTest::gtest_main)
target_compile_definitions(hcad_tests PRIVATE HCAD_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/data")
include(GoogleTest)
gtest_discover_tests(hcad_tests)

# UI smoke test: real window, real OCCT picking, screenshots. Needs a display
# (CI runs it under xvfb-run on Linux and on the desktop session on Windows).
if(TARGET hcad_ui)
  add_executable(hcad_ui_tests test_ui.cpp)
  target_link_libraries(hcad_ui_tests PRIVATE hcad_ui GTest::gtest)
  gtest_discover_tests(hcad_ui_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" DISCOVERY_MODE PRE_TEST)
endif()

if(TARGET hcad_app)
  find_program(XVFB_RUN xvfb-run)
  set(_smoke_cmd $<TARGET_FILE:hcad_app> --smoke-test ${CMAKE_BINARY_DIR}/screenshots
                 --mesh ${CMAKE_CURRENT_SOURCE_DIR}/data/scan_sphere.obj)
  if(WIN32)
    add_test(NAME ui_smoke COMMAND ${_smoke_cmd})
  elseif(XVFB_RUN)
    add_test(NAME ui_smoke COMMAND ${XVFB_RUN} -a -s "-screen 0 1600x1000x24" ${_smoke_cmd})
  endif()
endif()
