execute_process(COMMAND bash -c "test_${PROJECT_NAME} & test_${PROJECT_NAME} & wait" RESULT_VARIABLE status)
if (status)
    message (FATAL_ERROR "test failed")
endif()