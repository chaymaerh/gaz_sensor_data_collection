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
 * @file	    utils.cpp
 * @date	    22 June 2022
 * @version		1.5.5
 * 
 * @brief    	utils
 *
 * 
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* own header include */
#include "utils.h"
#include <math.h>

uint64_t 	utils::_tickMs;
uint64_t 	utils::_tickOverFlowCnt;
// SDFS		utils::_sd = SD;
SPIClass* 	utils::hspi = NULL;
RTC_PCF8523 utils::_rtc;
char 		utils::_fileSeed[DATA_LOG_FILE_SEED_SIZE];

/*!
 * @brief This function creates the random alphanumeric file seed for the log file
 */
void utils::createFileSeed()
{
	/* setup an alphanumeric characters buffer to choose from */
	const char *letters = "abcdefghijklmnopqrstuvwxyz0123456789";
	/* for each character of the file seed, randomly select one from the buffer */
	for (int seedChar = 0; seedChar < 16; seedChar++)
	{
		int randC = random(0, 36);
		_fileSeed[seedChar] = letters[randC];
	}
}

/*!
 * @brief This function initializes the module
 */	
demoRetCode utils::begin()
{
	demoRetCode retCode = EDK_OK;

	Wire.begin(I2C_SDA,I2C_SCL);

	if (utils::hspi == NULL){
		utils::hspi = new SPIClass(HSPI);
		utils::hspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);
		pinMode(PIN_SD_CS, OUTPUT);
	}	
	
	if (!SD.begin(PIN_SD_CS, *utils::hspi, SPI_SPEED_COM))
	{
		retCode = EDK_SD_CARD_INIT_ERROR;
	}
	else if (!_rtc.begin())
	{
		retCode = EDK_DATALOGGER_RTC_BEGIN_WARNING;
	}
	else if (!_rtc.initialized() || _rtc.lostPower())
	{
		_rtc.adjust(DateTime(F(__DATE__), F(__TIME__)) - TimeSpan((int32_t)(TIMEZONE * 3600)));
		
		retCode = EDK_DATALOGGER_RTC_ADJUST_WARNING;
	}
	randomSeed(analogRead(PIN_TO_GENARATE_RANDOM_SEED));
	createFileSeed();

	return retCode;
}

/*!
 * @brief This function retrieves the rtc handle
 */	
RTC_PCF8523& utils::getRtc()
{
	return _rtc;
}

/*!
 * @brief This function retrieves the created file seed
 */	
String utils::getFileSeed()
{
	return String(_fileSeed);
}

/*!
 * @brief This function creates a mac address string
 */
String utils::getMacAddress()
{
	uint64_t mac = ESP.getEfuseMac();
	char *macPtr = (char*)&mac;
	char macStr[13];
	
	sprintf(macStr, "%02X%02X%02X%02X%02X%02X", macPtr[0], macPtr[1], macPtr[2], macPtr[3], macPtr[4], macPtr[5]);
	
	return String(macStr);
}

/*!
 * @brief This function creates a date string
 */
String utils::getDateTime()
{
	char timeBuffer[20];
	DateTime date = _rtc.now();
	
	sprintf(timeBuffer, "%d_%02d_%02d_%02d_%02d", date.year(), date.month(), date.day(), date.hour(), date.minute());
	
	return String(timeBuffer);
}

/*!
 * @brief This function retrieves the first file with provided file extension
 */
bool utils::getFileWithExtension(String& fName, const String& extension)
{
	File root = SD.open("/");
	File file;
	// char fileName[90];
		
	if (root && root.isDirectory())
	{
		file = root.openNextFile();
		while (file)
		{
			if (!file.isDirectory())
			{
				const char* fileName = file.name();
				if (String(fileName).endsWith(extension))
				{
					file.close();
					
					fName = String(fileName);
					return true;
				}
			}
			file = root.openNextFile();  
		}
	}
	file.close();
	return false;
}

/*!
 * @brief This function retrives the bsec configuration string from the provided file
 */
demoRetCode utils::getBsecConfig(const String& fileName, uint8_t configStr[BSEC_MAX_PROPERTY_BLOB_SIZE])
{
	demoRetCode retCode = EDK_OK;
	uint32_t configStrLen;
	
	File configFile = SD.open(fileName, FILE_WRITE);
	if (!configFile)
	{
		retCode = EDK_BSEC_CONFIG_STR_FILE_ERROR;
	}
	configStrLen = configFile.size();
	if (configStrLen != BSEC_MAX_PROPERTY_BLOB_SIZE)
	{
		retCode = EDK_BSEC_CONFIG_STR_SIZE_ERROR;
	}
	else if (configFile.read(configStr, configStrLen) != configStrLen)
	{
		retCode = EDK_BSEC_CONFIG_STR_READ_ERROR;
	}
	return retCode;
}

/*!
 * @brief This function returns the tick value (ms)
 */
uint64_t utils::getTickMs(void)
{
	uint64_t timeMs = millis(); /* An overflow occurred */
	if (_tickMs > timeMs) 
	{ 
		_tickOverFlowCnt++;
	}
	_tickMs = timeMs;
	return timeMs + (_tickOverFlowCnt * INT64_C(0xFFFFFFFF));
}
