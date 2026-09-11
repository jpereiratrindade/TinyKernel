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
  src/experiment/workflow.cpp
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

if(BUILD_TESTING)
  add_executable(tinykernel-tests tests/core_tests.cpp)
  target_link_libraries(tinykernel-tests PRIVATE TinyKernel::core)

  foreach(case_name IN ITEMS
      identity graph interventions persistence evidence_immutable adjudication
      claim_limits frontier deterministic_export tk0001_e2e
      sait_benchmark epistemic_non_implication
      persistence_evidence_type_roundtrip export_evidence_type
      epistemic_phase_separation adjudication_provenance_run_id
      l3_strict_causal_gate schema_migration_v1_to_v2
      monotonic_phase_machine partial_observation_inconclusive
      delete_investigation_safe_lifecycle evidence_envelope_immutability
      web_workflow_transactions)
    add_test(NAME "core.${case_name}" COMMAND tinykernel-tests "${case_name}")
  endforeach()

  find_program(NODE_EXECUTABLE node)
  if(NODE_EXECUTABLE)
    add_test(NAME web.epistemic_engine
      COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/web_engine_tests.js)
  endif()

  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_Interpreter_FOUND)
    add_test(NAME web.api_bridge
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/web_api_tests.py)
  endif()

  add_test(NAME cli.commands
    COMMAND ${CMAKE_COMMAND}
      -DCLI=$<TARGET_FILE:tinykernel-cli>
      -DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/cli-test-workspace
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_smoke.cmake)

endif()
