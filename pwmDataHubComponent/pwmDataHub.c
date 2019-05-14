/**
 * Description: This file is migrated from https:github.com/Seeed-Studio/Seeed_RGB_LED_Matrix
 * 		Some functions have been renamed but its code still be the same
 * Author: tqkieu@tma.com.vn
 * 
 **/

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/io.h>
#include <stdio.h>
#include <errno.h>

#include "legato.h"
#include "interfaces.h"
#include "le_mutex.h"
#include "json.h"

#define PWMDATAHUB_DATAHUB_SERVO	"pwdDataHub/servo"

le_mutex_Ref_t pwdDataHub_lock = NULL;

static void json_extract_dump(le_result_t res)
{
	if (res == LE_OK) {
		LE_INFO("json_Extract: successful");
	}
	if (res == LE_FORMAT_ERROR) {
		LE_ERROR("json_Extract: there's something wrong with the input JSON string.");
	}
	if (res == LE_BAD_PARAMETER) {
		LE_ERROR("json_Extract: there's something wrong with the extraction specification");
	}
	if (res == LE_NOT_FOUND) {
		LE_ERROR("json_Extract: the thing we are trying to extract doesn't exist in the JSON input");
	}
	if (res == LE_OVERFLOW) {
		LE_ERROR("json_Extract: the provided result buffer isn't big enough");
	}
}
static void pwdDataHubServoHandle(double timestamp,
				  const char* LE_NONNULL value,
				  void* contextPtr)
{
	char buffer[IO_MAX_STRING_VALUE_LEN];
	int interface = 0;
	int angle = 0;
	le_result_t le_res;
	json_DataType_t json_data_type;

	LE_INFO("pwdDataHubServoHandle: timestamp %lf", timestamp);
	LE_INFO("pwdDataHubServoHandle: value %s", value);

	// JSON message looks like this:
	// "
	// {
	// 	\"interface\": \"1\",
	// 	\"angle\": \"90\"
	// }
	// "
	if (!json_IsValid(value)) {
		LE_ERROR("INVALID JSON string");
		return;
	}

	// interface
	memset(buffer, 0, IO_MAX_STRING_VALUE_LEN);
	le_res = json_Extract(buffer,
			      IO_MAX_STRING_VALUE_LEN,
			      value,
			      "interface", 
			      &json_data_type);
	json_extract_dump(le_res);
	if (json_data_type != JSON_TYPE_NUMBER) {
		LE_ERROR("WRONG data type for interface");
		return;
	}
	interface = (int)json_ConvertToNumber(buffer);
	
	// angle
	memset(buffer, 0, IO_MAX_STRING_VALUE_LEN);
	le_res = json_Extract(buffer,
			      IO_MAX_STRING_VALUE_LEN,
			      value,
			      "angle", 
			      &json_data_type);
	json_extract_dump(le_res);
	if (json_data_type != JSON_TYPE_NUMBER) {
		LE_ERROR("WRONG data type for angle");
		return;
	}
	angle = (int)json_ConvertToNumber(buffer);

	le_mutex_Lock(pwdDataHub_lock);
	ma_pwm_set_angle(interface, angle);
	le_mutex_Unlock(pwdDataHub_lock);
}

COMPONENT_INIT
{
	le_result_t result;
	pwdDataHub_lock = le_mutex_CreateNonRecursive("pwdDataHub");

	// servo pwd initial
	ma_pwm_servo_init();

	// This will be received from the Data Hub.
	result = io_CreateOutput(PWMDATAHUB_DATAHUB_SERVO,
				 IO_DATA_TYPE_JSON,
				 "servo");
	LE_ASSERT(result == LE_OK);

	// Register for notification of updates to the counter value.
	io_AddJsonPushHandler(PWMDATAHUB_DATAHUB_SERVO,
			      pwdDataHubServoHandle,
			      NULL);
}