if(NOT DEFINED GUI OR NOT DEFINED TEST_ROOT)
  message(FATAL_ERROR "GUI and TEST_ROOT are required")
endif()

file(REMOVE_RECURSE "${TEST_ROOT}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env
    QT_QPA_PLATFORM=offscreen QSG_RHI_BACKEND=software
    "${GUI}" --workspace "${TEST_ROOT}" --exercise --smoke
  RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT status EQUAL 0)
  message(FATAL_ERROR "GUI workflow failed\n${output}\n${error}")
endif()
if(NOT EXISTS "${TEST_ROOT}/tinykernel.sqlite3")
  message(FATAL_ERROR "GUI did not persist the investigation")
endif()
if(NOT EXISTS "${TEST_ROOT}/TK-0001.json")
  message(FATAL_ERROR "GUI did not export JSON")
endif()
file(READ "${TEST_ROOT}/TK-0001.json" exported)
if(NOT exported MATCHES "tinykernel-investigation-json")
  message(FATAL_ERROR "GUI export has an invalid envelope")
endif()
file(REMOVE_RECURSE "${TEST_ROOT}")
