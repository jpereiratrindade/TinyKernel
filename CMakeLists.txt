cmake_minimum_required(VERSION 3.28)

project(TinyKernel VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(CTest)
find_package(SQLite3 3.35 REQUIRED)

add_library(libtinykernel
  src/ontology/types.cpp
  src/causal/engine.cpp
  src/experiment/laboratory.cpp
  src/evidence/digest.cpp
  src/knowledge/analysis.cpp
  src/persistence/repository.cpp
  src/persistence/export.cpp
)
set_target_properties(libtinykernel PROPERTIES OUTPUT_NAME tinykernel)
add_library(TinyKernel::core ALIAS libtinykernel)
target_include_directories(libtinykernel PUBLIC include)
target_link_libraries(libtinykernel PUBLIC SQLite3::SQLite3)
target_compile_features(libtinykernel PUBLIC cxx_std_26)
target_compile_definitions(libtinykernel PUBLIC
  TINYKERNEL_VERSION="${PROJECT_VERSION}"
  TINYKERNEL_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(libtinykernel PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

add_executable(tinykernel-cli src/cli/main.cpp)
set_target_properties(tinykernel-cli PROPERTIES OUTPUT_NAME tinykernel)
target_link_libraries(tinykernel-cli PRIVATE TinyKernel::core)

option(TINYKERNEL_BUILD_GUI "Build the Qt Quick interface" ON)
if(TINYKERNEL_BUILD_GUI)
  find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
  qt_standard_project_setup(REQUIRES 6.5)
  qt_policy(SET QTP0004 NEW)
  qt_add_executable(tinykernel-gui src/gui/main.cpp src/gui/gui_bridge.cpp src/gui/gui_bridge.hpp)
  target_include_directories(tinykernel-gui PRIVATE src/gui)
  target_link_libraries(tinykernel-gui PRIVATE
    TinyKernel::core Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)
  qt_add_qml_module(tinykernel-gui
    URI TinyKernel
    VERSION 1.0
    NO_CACHEGEN
    QML_FILES qml/Main.qml
  )
endif()

if(BUILD_TESTING)
  add_executable(tinykernel-tests tests/core_tests.cpp)
  target_link_libraries(tinykernel-tests PRIVATE TinyKernel::core)

  foreach(case_name IN ITEMS
      identity graph interventions persistence evidence_immutable adjudication
      claim_limits frontier deterministic_export tk0001_e2e
      sait_benchmark epistemic_non_implication
      persistence_evidence_type_roundtrip export_evidence_type
      epistemic_phase_separation adjudication_provenance_run_id
      l3_strict_causal_gate)
    add_test(NAME "core.${case_name}" COMMAND tinykernel-tests "${case_name}")
  endforeach()

  add_test(NAME cli.commands
    COMMAND ${CMAKE_COMMAND}
      -DCLI=$<TARGET_FILE:tinykernel-cli>
      -DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/cli-test-workspace
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_smoke.cmake)

  if(TINYKERNEL_BUILD_GUI)
    add_test(NAME gui.startup COMMAND tinykernel-gui --smoke)
    set_tests_properties(gui.startup PROPERTIES
      ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QSG_RHI_BACKEND=software")
    add_test(NAME gui.workflow
      COMMAND ${CMAKE_COMMAND}
        -DGUI=$<TARGET_FILE:tinykernel-gui>
        -DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/gui-test-workspace
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/gui_workflow.cmake)
  endif()
endif()
