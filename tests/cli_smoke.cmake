if(NOT DEFINED CLI OR NOT DEFINED TEST_ROOT)
  message(FATAL_ERROR "CLI and TEST_ROOT are required")
endif()

file(REMOVE_RECURSE "${TEST_ROOT}")

function(run_cli)
  execute_process(COMMAND "${CLI}" ${ARGN}
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(NOT status EQUAL 0)
    message(FATAL_ERROR "Command failed: ${CLI} ${ARGN}\n${output}\n${error}")
  endif()
endfunction()

run_cli(version)
run_cli(init "${TEST_ROOT}")
run_cli(--workspace "${TEST_ROOT}" list)
run_cli(--workspace "${TEST_ROOT}" show TK-0001)
run_cli(--workspace "${TEST_ROOT}" run TK-0001)
run_cli(--workspace "${TEST_ROOT}" --json frontier TK-0001)
run_cli(--workspace "${TEST_ROOT}" --json claims TK-0001)
run_cli(--workspace "${TEST_ROOT}" export TK-0001)

file(REMOVE_RECURSE "${TEST_ROOT}")
