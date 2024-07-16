/**
 * Copyright (c) 2021 Bosch Sensortec GmbH. All rights reserved.
 *
 * BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @file	bme68x_demo_sample.ino
 * @date	22 June 2022
 * @version	1.5.5
 * 
 * 
 */

/* The new sensor needs to be conditioned before the example can work reliably. You may 
 * run this example for 24hrs to let the sensor stabilize.
 */

/**
 * bme68x_demo_sample.ino :
 * This is an example code for datalogging and integration of BSEC2x library in BME688 development kit
 * which has been designed to work with Adafruit ESP32 Feather Board
 * For more information visit : 
 * https://www.bosch-sensortec.com/software-tools/software/bme688-software/
 */
#include <Arduino.h>
#include <bme68x_datalogger.h>
#include <bsec_datalogger.h>
#include <label_provider.h>
#include <led_controller.h>
#include <sensor_manager.h>
// #include <ble_controller.h>
#include <bsec2.h>
#include <utils.h>
#include <pins_arduino.h>


#include <soc/soc.h>                                                // desable brownout problems
#include <soc/rtc_cntl_reg.h>                                        // desable brownout problems

// #define LOG_DEBUG

#ifdef LOG_DEBUG
#define SERIAL_PRINTLN(msg)  (Serial.println(msg))
#define SERIAL_PRINT(msg)  (Serial.print(msg))
#else
#define SERIAL_PRINTLN(msg)
#define SERIAL_PRINT(msg)
#endif

/*! BUFF_SIZE determines the size of the buffer */
#define BUFF_SIZE 10

/*!
 * @brief : This function is called by the BSEC library when a new output is available
 *
 * @param[in] input 	: BME68X data
 * @param[in] outputs	: BSEC output data
 */
// void bsecCallBack(const bme68x_data input, const bsecOutputs outputs);
void bsecCallBack(const bme68x_data input, const bsecOutputs outputs, Bsec2 bsec);

/*!
 * @brief : This function handles sensor manager and BME68X datalogger configuration
 *
 * @param[in] bmeExtension : reference to the bmeconfig file extension
 *
 * @return  Application return code
 */
demoRetCode configureSensorLogging(const String& bmeExtension);

/*!
 * @brief : This function handles BSEC datalogger configuration
 *
 * @param[in] bsecExtension		 : reference to the BSEC configuration string file extension
 * @param[inout] bsecConfigStr	 : pointer to the BSEC configuration string
 *
 * @return  Application return code
 */
demoRetCode configureBsecLogging(const String& bsecExtension, uint8_t bsecConfigStr[BSEC_MAX_PROPERTY_BLOB_SIZE]);

uint8_t 				bsecConfig[BSEC_MAX_PROPERTY_BLOB_SIZE];
Bsec2 					bsec2;
// bleController  			bleCtlr(bleMessageReceived);
labelProvider 			labelPvr;
ledController			ledCtlr;
sensorManager 			sensorMgr;
bme68xDataLogger		bme68xDlog;
bsecDataLogger 			bsecDlog;
demoRetCode				retCode;
uint8_t					bsecSelectedSensor;
String 					bme68xConfigFile, bsecConfigFile;
demoAppMode				appMode;
gasLabel 				label;
bool 					isBme68xConfAvailable, isBsecConfAvailable;
commMux					comm;

static volatile uint8_t buffCount = 0;
static bsecDataLogger::SensorIoData buff[BUFF_SIZE];

void setup()
{
	#ifdef LOG_DEBUG
	Serial.begin(115200);
	while (!Serial.available()){}
	Serial.read();
	SERIAL_PRINTLN("Check point 0");
	#endif

	/**********************************************   Disable brownout detectore   ********************************************/
  	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
	
	/* Datalogger Mode is set by default */
	appMode = DEMO_DATALOGGER_MODE;
	label = BSEC_NO_CLASS;
	bsecSelectedSensor = 0;
	/* Initializes the label provider module */
	labelPvr.begin();
	SERIAL_PRINTLN("Check point 10");
	/* Initializes the led controller module */
    ledCtlr.begin();    
	SERIAL_PRINTLN("Check point 11");
	/* Initializes the SD and RTC module */
	retCode = utils::begin();

	SERIAL_PRINTLN("Check point 1");

	if (retCode >= EDK_OK)
	{
        /* checks the availability of BME board configuration and BSEC configuration files */
		isBme68xConfAvailable = utils::getFileWithExtension(bme68xConfigFile, BME68X_CONFIG_FILE_EXT);
		isBsecConfAvailable = utils::getFileWithExtension(bsecConfigFile, BSEC_CONFIG_FILE_EXT);

		SERIAL_PRINTLN(bme68xConfigFile[0]);
		if (bme68xConfigFile[0] != '/') bme68xConfigFile = String("/") + bme68xConfigFile;
		
		if (isBme68xConfAvailable)
		{
			retCode = configureSensorLogging(bme68xConfigFile);
		}
		else
		{
			retCode = EDK_SENSOR_CONFIG_FILE_ERROR;
		}
	}
	SERIAL_PRINTLN("Check point 2");
	if (retCode < EDK_OK)
	{
		if (retCode != EDK_SD_CARD_INIT_ERROR)
		{
			/* creates log file and updates the error codes */
			if (bme68xDlog.begin(bme68xConfigFile) != EDK_SD_CARD_INIT_ERROR)
			/* Writes the sensor data to the current log file */
			(void) bme68xDlog.writeSensorData(nullptr, nullptr, nullptr, nullptr, label, retCode);
			/* Flushes the buffered sensor data to the current log file */
			(void) bme68xDlog.flush();
		}
		appMode = DEMO_IDLE_MODE;
	}
	SERIAL_PRINTLN("Check point 3");
	bsec2.attachCallback(bsecCallBack);
}

void loop() 
{
	/* Updates the led controller status */
	ledCtlr.update(retCode);
	if (retCode >= EDK_OK)
	{
		/* Retrieves the current label */
		(void) labelPvr.getLabel(label);
		switch (appMode)
		{
			/*  Logs the bme688 sensors raw data from all 8 sensors */
			case DEMO_DATALOGGER_MODE:
			{
				uint8_t i;
				// SERIAL_PRINTLN("1");
                /* Schedules the next readable sensor */
				while (sensorMgr.scheduleSensor(i))
				{
					bme68x_data* sensorData[3];
					/* Returns the selected sensor address */
                    bme68xSensor* sensor = sensorMgr.getSensor(i);
					/* Retrieves the selected sensor data */
					retCode = sensorMgr.collectData(i, sensorData);
					if (retCode < EDK_OK)
					{
						/* Writes the sensor data to the current log file */
						retCode = bme68xDlog.writeSensorData(&i, &sensor->id, &sensor->mode, nullptr, label, retCode);
					}
					else
					{
						for (const auto data : sensorData)
						{
							if (data != nullptr)
							{
								retCode = bme68xDlog.writeSensorData(&i, &sensor->id, &sensor->mode, data, label, retCode);
							}
						}
					}
				}
				
				retCode = bme68xDlog.flush();
			}
			break;
			/* Example of BSEC library integration: gets the data from one out of 8 sensors
			   (this can be selected through application) and calls BSEC library,
			   get the outputs in app and logs the data */
			case DEMO_BLE_STREAMING_MODE:
			{
				/* Callback from the user to read data from the BME688 sensors using parallel mode/forced mode,
				   process and store outputs */
				(void) bsec2.run();
			}
			break;
			default:
			break;
		}
	}
	else
	{
		SERIAL_PRINTLN("Error code = " + String((int) retCode));
		while(1)
		{
			/* Updates the led controller status */
			ledCtlr.update(retCode);
		}
	}
}

void bsecCallBack(const bme68x_data input, const bsecOutputs outputs, Bsec2 bsec)
{ 
	// bleNotifyBme68xData(input);	
	// if (outputs.nOutputs)
	// {
	// 	bleNotifyBsecOutput(outputs);
	// }

	bme68xSensor *sensor = sensorMgr.getSensor(bsecSelectedSensor); /* returns the selected sensor address */
	
	if (sensor != nullptr)
	{
		buff[buffCount].sensorNum = bsecSelectedSensor;
		buff[buffCount].sensorId = sensor->id;
		buff[buffCount].sensorMode = sensor->mode;
		buff[buffCount].inputData = input;
		buff[buffCount].outputs = outputs;
		buff[buffCount].label = label;
		buff[buffCount].code = retCode;
		buff[buffCount].timeSincePowerOn = millis();
		buff[buffCount].rtcTsp = utils::getRtc().now().unixtime();
		buffCount ++;  
		
		if (buffCount == BUFF_SIZE)
		{
			retCode = bsecDlog.writeBsecOutput(buff, BUFF_SIZE);
			buffCount = 0;
		}
	}
}

demoRetCode configureSensorLogging(const String& bmeConfigFile)
{
	demoRetCode ret = sensorMgr.begin(bmeConfigFile);
	if (ret >= EDK_OK)
	{
		ret = bme68xDlog.begin(bmeConfigFile);
	}
	return ret;
}

demoRetCode configureBsecLogging(const String& bsecConfigFile, uint8_t bsecConfigStr[BSEC_MAX_PROPERTY_BLOB_SIZE])
{
	memset(bsecConfigStr, 0, BSEC_MAX_PROPERTY_BLOB_SIZE);
	demoRetCode ret = bsecDlog.begin(bsecConfigFile);
	if (ret >= EDK_OK)
	{
		ret = utils::getBsecConfig(bsecConfigFile, bsecConfigStr);
	}
	return ret;
}
