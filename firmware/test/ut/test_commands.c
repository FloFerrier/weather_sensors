#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <cmocka.h>

#include "mock_sensor.h"
#include "mock_console.h"
#include "commands.h"
#include "bme68x.h"

#define MESSAGE_LEN_MAX (225u)

static void test_command_get_index_should_be_help() {
    command_index_e command_is_available = command_getIndex("help");

    assert_int_equal(command_is_available, COMMAND_HELP);
}

static void test_command_get_index_should_be_unknown() {
    command_index_e command_is_available = command_getIndex("");

    assert_int_equal(command_is_available, COMMAND_UNKNOWN);
}

static void test_command_should_avoid_incorrect_parameter() {
    command_index_e command_is_available = command_getIndex(NULL);

    assert_int_equal(command_is_available, COMMAND_UNKNOWN);
}

static void test_command_execute_should_avoid_incorrect_parameter() {
    bool command_is_executed = command_execute(-1);

    assert_int_equal(command_is_executed, false);
}

static void test_command_execute_should_be_unknown_command() {
    mock_assert_call_console_send("{\"code\":400, \"message\":\"Bad request\", \"response\":\"Tap help to display all available command.\"}\r\n", true);

    bool command_is_executed = command_execute(COMMAND_UNKNOWN);

    assert_int_equal(command_is_executed, true);
}

static void test_command_execute_should_display_all_available_command() {
    mock_assert_call_console_send("{\"code\":200, \"message\":\"Success\", \"response\":\"\"}\r\n", true);

    bool command_is_executed = command_execute(COMMAND_HELP);

    assert_int_equal(command_is_executed, true);
}

static void test_command_execute_should_failed_sensor_selftest() {
    mock_assert_call_sensor_selfTest(BME68X_E_SELF_TEST);
    mock_assert_call_console_send("{\"code\":503, \"message\":\"Service Unavailable\", \"response\":\"\"}\r\n", true);

    bool command_is_executed = command_execute(COMMAND_SENSOR_SELF_TEST);

    assert_int_equal(command_is_executed, true);
}

static void test_command_execute_should_passed_sensor_selftest() {
    mock_assert_call_sensor_selfTest(BME68X_OK);
    mock_assert_call_console_send("(sensor)> Self-test passed !\r\n", true);

    bool command_is_executed = command_execute(COMMAND_SENSOR_SELF_TEST);

    assert_int_equal(command_is_executed, true);
}

static void test_command_execute_should_failed_sensor_get_data() {
    sensor_data_s expected_data = {0};
    uint32_t expected_number_of_data = 0;
    mock_assert_call_sensor_getData(&expected_data, expected_number_of_data, BME68X_W_NO_NEW_DATA);
    mock_assert_call_console_send("{\"code\":503, \"message\":\"Service Unavailable\", \"response\":\"\"}\r\n", true);

    bool command_is_executed = command_execute(COMMAND_SENSOR_GET_DATA);

    assert_int_equal(command_is_executed, true);
}

static void test_command_execute_should_passed_sensor_get_data() {
    sensor_data_s expected_data[SENSOR_MAX_DATA_AVAILABLE] = {
        {.temperature_in_deg = 15.0f, .pressure_in_pascal = 98000.0f, .humidity_in_per100 = 50.0f, .gas_resistance_in_ohms = 0.0f},
        {.temperature_in_deg = 20.0f, .pressure_in_pascal = 99000.0f, .humidity_in_per100 = 55.0f, .gas_resistance_in_ohms = 0.0f},
        {.temperature_in_deg = 25.0f, .pressure_in_pascal = 100000.0f, .humidity_in_per100 = 60.0f, .gas_resistance_in_ohms = 0.0f},
        {.temperature_in_deg = 30.0f, .pressure_in_pascal = 110000.0f, .humidity_in_per100 = 65.0f, .gas_resistance_in_ohms = 0.0f},
    };
    const char expected_message[SENSOR_MAX_DATA_AVAILABLE][MESSAGE_LEN_MAX] = {
        "(sensor)> Temperature 15.0 deg, Pressure 980.0 hPa, Humidity 50.0 %rH\r\n",
        "(sensor)> Temperature 20.0 deg, Pressure 990.0 hPa, Humidity 55.0 %rH\r\n",
        "(sensor)> Temperature 25.0 deg, Pressure 1000.0 hPa, Humidity 60.0 %rH\r\n",
        "(sensor)> Temperature 30.0 deg, Pressure 1100.0 hPa, Humidity 65.0 %rH\r\n",
    };
    uint32_t expected_number_of_data = sizeof(expected_data) / sizeof(sensor_data_s);
    mock_assert_call_sensor_getData(expected_data, expected_number_of_data, BME68X_OK);
    mock_assert_call_console_send("(sensor)> Number of data available 4\r\n", true);
    for(uint32_t index=0; index<expected_number_of_data; index++) {
        mock_assert_call_console_send(expected_message[index], true);
    }

    bool command_is_executed = command_execute(COMMAND_SENSOR_GET_DATA);

    assert_int_equal(command_is_executed, true);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_command_get_index_should_be_help),
        cmocka_unit_test(test_command_get_index_should_be_unknown),
        cmocka_unit_test(test_command_should_avoid_incorrect_parameter),
        cmocka_unit_test(test_command_execute_should_avoid_incorrect_parameter),
        cmocka_unit_test(test_command_execute_should_be_unknown_command),
        cmocka_unit_test(test_command_execute_should_display_all_available_command),
        cmocka_unit_test(test_command_execute_should_failed_sensor_selftest),
        cmocka_unit_test(test_command_execute_should_passed_sensor_selftest),
        cmocka_unit_test(test_command_execute_should_failed_sensor_get_data),
        cmocka_unit_test(test_command_execute_should_passed_sensor_get_data),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}