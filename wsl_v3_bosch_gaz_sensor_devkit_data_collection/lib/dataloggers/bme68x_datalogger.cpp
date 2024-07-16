/*!
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
 * @file	    bme68x_datalogger.cpp
 * @date	    22 June 2022
 * @version		1.5.5
 * 
 * @brief    	bme68x_datalogger
 *
 * 
 */

/* own header include */
#include "bme68x_datalogger.h"
#include <Esp.h>

/*!
 * @brief The constructor of the bme68xDataLogger class
 */
bme68xDataLogger::bme68xDataLogger() : _fileCounter(0)
{}

/*!
 * @brief Function to configure the datalogger using the provided sensor config file
 */
demoRetCode bme68xDataLogger::begin(const String& configName)
{
	demoRetCode retCode = utils::begin();
	
	_configName = configName;
	if (retCode >= EDK_OK)
	{
		_saveDataPos = false;
		retCode = createFile(_logFileName);
		retCode = createFile(_tempLogFile);
		
		_ss.setf(std::ios::fixed, std::ios::floatfield);
	}
	return retCode;
}

/*!
 * @brief Function to commit data. temp file is used to unsure the retention of the data, due to the library SD who lost all data if the file is not closed
 */
unsigned long bme68xDataLogger::commitLog(unsigned long pos, String logFileName, const char* logData){
	File logFile = SD.open(logFileName, FILE_WRITE);
	if(!logFile){
		return pos;
	}
	logFile.seek(pos);
	logFile.print(logData);
	unsigned long new_pos = logFile.position();
	logFile.println(END_OF_FILE);
	logFile.close();
	return new_pos;
}

/*!
 * @brief Function which flushes the buffered sensor data to the current log file
 */
demoRetCode bme68xDataLogger::flush()
{
	demoRetCode retCode = EDK_OK;
	std::string txt;
	
    if (_ss.rdbuf()->in_avail())
	{
		txt = _ss.str();
		_ss.str(std::string());
		
		if (_fileCounter)
		{
			commitLog(_sensorDataPos, _logFileName, txt.c_str());
			_sensorDataPos = commitLog(_sensorDataPos, _tempLogFile, txt.c_str());

			if (_sensorDataPos >= FILE_SIZE_LIMIT)
			{
				_saveDataPos = false;
				retCode = createFile(_logFileName);
				retCode = createFile(_tempLogFile);
			}
		}
		else
		{
			retCode = EDK_DATALOGGER_LOG_FILE_ERROR;
		}
	}
	return retCode;
}

/*!
 * @brief Function writes the sensor data to the current log file
 */
demoRetCode bme68xDataLogger::writeSensorData(const uint8_t* num, const uint32_t* sensorId, const uint8_t* sensorMode, const bme68x_data* bme68xData, gasLabel label, demoRetCode code)
{
	demoRetCode retCode = EDK_OK;
    uint32_t rtcTsp = utils::getRtc().now().unixtime();
    uint32_t timeSincePowerOn = millis();
	if (_endOfLine)
	{
		_ss << ",\n";
	}
	
	_ss << "\t\t[";
	(num != nullptr) ? (_ss << (int)*num) : (_ss << "null");
	_ss << ",";
	(sensorId != nullptr) ? (_ss << (int)*sensorId) : (_ss << "null");
	_ss << ",";
	_ss << timeSincePowerOn;
	_ss << ",";
	_ss << rtcTsp;
	_ss << ",";
	(bme68xData != nullptr) ? (_ss << bme68xData->temperature) : (_ss << "null");
	_ss << ",";
	(bme68xData != nullptr) ? (_ss << (bme68xData->pressure * .01f)) : (_ss << "null");
	_ss << ",";
	(bme68xData != nullptr) ? (_ss << bme68xData->humidity) : (_ss << "null");
	_ss << ",";
	(bme68xData != nullptr) ? (_ss << bme68xData->gas_resistance) : (_ss << "null");
	_ss << ",";
	(bme68xData != nullptr) ? (_ss << (int)bme68xData->gas_index) : (_ss << "null");
	_ss << ",";
	(sensorMode != nullptr) ? (_ss << (int)(*sensorMode == BME68X_PARALLEL_MODE)) : (_ss << "null");
	_ss << ",";
	_ss << (int)label;
	_ss << ",";
	_ss << (int)code;
	_ss << "]";
	_endOfLine = true;
    return retCode;
}

/*!
 * @brief function to create a bme68x datalogger output file with .bmerawdata extension
 */
demoRetCode bme68xDataLogger::createFile(String &fileName)
{
	demoRetCode retCode = EDK_OK;
	String macStr = utils::getMacAddress();
    String logFileBaseName = "_Board_" + macStr + "_PowerOnOff_1_";
	
    fileName = "/" + utils::getDateTime() + logFileBaseName + utils::getFileSeed() + "_File_" + String(_fileCounter) + BME68X_RAWDATA_FILE_EXT;             

	File configFile = SD.open(_configName, FILE_READ);
	File file = SD.open(fileName, FILE_WRITE);
    if (_configName.length() && !configFile)
	{
		retCode = EDK_DATALOGGER_SENSOR_CONFIG_FILE_ERROR;
	}
	else if (!file)
	{
		retCode = EDK_DATALOGGER_LOG_FILE_ERROR;
	}
	else
	{
		if (_configName.length())
		{
			String lineBuffer;
			/* read in each line from the config file and copy it to the log file */
			while(configFile.available())
			{
				lineBuffer = configFile.readStringUntil('\n');
				/* skip the last closing curly bracket of the JSON document */
				if (lineBuffer == "}")
				{
					file.println("\t,");
					break;
				}
				file.println(lineBuffer);
			}
			configFile.close();
		}
		else
		{
			file.println("{");
		}

		/* write data header / skeleton */
		/* raw data header */
		file.println("    \"rawDataHeader\":");		
		file.println("\t{");
		file.println("\t    \"counterPowerOnOff\": 1,");
		file.println("\t    \"seedPowerOnOff\": \"" + utils::getFileSeed() + "\",");
		file.println("\t    \"counterFileLimit\": " + String(_fileCounter) + ",");
		file.println("\t    \"dateCreated\": \"" + String(utils::getRtc().now().unixtime()) + "\",");
		file.println("\t    \"dateCreated_ISO\": \"" + utils::getRtc().now().timestamp() + "+00:00\",");
		file.println("\t    \"firmwareVersion\": \"" + String(FIRMWARE_VERSION) + "\",");
		file.println("\t    \"boardId\": \"" + macStr + "\"");
		file.println("\t},");
		file.println("    \"rawDataBody\":");
		file.println("\t{");
		file.println("\t    \"dataColumns\": [");
		file.println("\t\t{");
		file.println("\t\t    \"name\": \"Sensor Index\",");
		file.println("\t\t    \"unit\": \"\",");
		file.println("\t\t    \"format\": \"integer\",");
		file.println("\t\t    \"key\": \"sensor_index\"");
		file.println("\t\t},");
		file.println("\t\t{");
		file.println("\t\t    \"name\": \"Sensor ID\",");
		file.println("\t\t    \"unit\": \"\",");
		file.println("\t\t    \"format\": \"integer\",");
		file.println("\t\t    \"key\": \"sensorId\"");
		file.println("\t\t},");
		file.println("\t\t{");
		file.println("\t\t    \"name\": \"Time Since PowerOn\",");
		file.println("\t\t    \"unit\": \"Milliseconds\",");
		file.println("\t\t    \"format\": \"integer\",");
		file.println("\t\t    \"key\": \"timestamp_since_poweron\"");
		file.println("\t\t},");
		file.println("\t\t{");
		file.println("\t\t    \"name\": \"Real time clock\",");
		file.println("\t\t    \"unit\": \"Unix Timestamp: seconds since Jan 01 1970. (UTC); 0 = missing\",");
		file.println("\t\t    \"format\": \"integer\",");
		file.println("\t\t    \"key\": \"real_time_clock\"");
		file.println("\t\t},");
		file.println("\t\t{");
		file.println("\t\t    \"name\": \"Temperature\",");
		file.println("\t\t    \"unit\": \"DegreesClecius\",");
		file.println("\t\t    \"format\": \"float\",");
		file.println("\t\t    \"key\": \"temperature\"");
		file.println("\t\t},");
		file.println("\t\t{");
		file.println("\t\t    \"name\": \"Pressure\",");
		file.println("\t\t    \"unit\": \"Hectopascals\",");
		file.println("\t\t    \"format\": \"float\",");
		file.println("\t\t    \"key\": \"pressure\"");
		file.println("\t\t},");
		file.println("\t\t{");
		file.println("\t\t    \"name\": \"Relative Humidity\",");
		file.println("\t\t    \"unit\": \"Percent\",");
		file.println("\t\t    \"format\": \"float\",");
		file.println("\t\t    \"key\": \"relative_humidity\"");
		file.println("\t\t},");
		file.println("\t\t{");
		file.println("\t\t    \"name\": \"Resistance Gassensor\",");
		file.println("\t\t    \"unit\": \"Ohms\",");
		file.println("\t\t    \"format\": \"float\",");
		file.println("\t\t    \"key\": \"resistance_gassensor\"");
		file.println("\t\t},");
		file.println("\t\t{");
		file.println("\t\t    \"name\": \"Heater Profile Step Index\",");
		file.println("\t\t    \"unit\": \"\",");
		file.println("\t\t    \"format\": \"integer\",");
		file.println("\t\t    \"key\": \"heater_profile_step_index\"");
		file.println("\t\t},");
		file.println("\t\t{");
		file.println("\t\t    \"name\": \"Scanning enabled\",");
		file.println("\t\t    \"unit\": \"\",");
		file.println("\t\t    \"format\": \"integer\",");
		file.println("\t\t    \"key\": \"scanning_enabled\"");
		file.println("\t\t},");
		file.println("\t\t{");
		file.println("\t\t    \"name\": \"Label Tag\",");
		file.println("\t\t    \"unit\": \"\",");
		file.println("\t\t    \"format\": \"integer\",");
		file.println("\t\t    \"key\": \"label_tag\"");
		file.println("\t\t},");
		file.println("\t\t{");
		file.println("\t\t    \"name\": \"Error Code\",");
		file.println("\t\t    \"unit\": \"\",");
		file.println("\t\t    \"format\": \"integer\",");
		file.println("\t\t    \"key\": \"error_code\"");
		file.println("\t\t}");
		file.println("\t    ],");
		
		/* data block */
		file.println("\t    \"dataBlock\": [");
		/* save position in file, where to write the first data set */
		if(_saveDataPos) _sensorDataPos = file.position();
		else _saveDataPos = true;
		file.println("\t    ]");
		file.println("\t}");
		file.println("}");

		// Serial.print("Position : ");
		// Serial.println(_sensorDataPos);
		
		/* close log file */
		file.close();
		
		_endOfLine = false;
		++_fileCounter;
	}
    return retCode;
}
