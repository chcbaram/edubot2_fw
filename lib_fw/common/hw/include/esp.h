/*
 * esp.h
 *
 *  Created on: 2021. 2. 13.
 *      Author: baram
 */

#ifndef SRC_COMMON_HW_INCLUDE_ESP_H_
#define SRC_COMMON_HW_INCLUDE_ESP_H_

#include "hw_def.h"


#ifdef _USE_HW_ESP


bool espInit(void);
bool espOpen(uint8_t ch, uint32_t baud);
bool espIsOpen(void);

uint32_t espAvailable(void);
uint32_t espWrite(uint8_t *p_data, uint32_t length);
uint8_t  espRead(void);
uint32_t espPrintf(char *fmt, ...);

bool espCmd(const char *cmd_str, uint32_t timeout);
void espLogEnable(void);
void espLogDisable(void);
bool espWaitOK(uint32_t timeout);

#endif


#endif /* SRC_COMMON_HW_INCLUDE_ESP_H_ */
