/*
 * gateway.h
 *
 *  Created on: Feb 22, 2019
 *      Author: dmartin
 */

#ifndef GATEWAY_H_
#define GATEWAY_H_

#include "main.h"

#define CMD_GW_SET_SYS_PRESSURE 0xA52A
#define CMD_GW_SET_SYS_VACUUM  0xA52D
/* The following struct is used for both the above commands */

struct __attribute__((packed)) DATA_GW_CMD_SET_PRESS {
	uint16_t lowerThreshold;
	uint16_t upperThreshold;
};

#define CMD_GW_SET_DECKLOAD 0xA537
//32 bit value 0/1

#define CMD_GW_SET_TOPLIGHT 0xA538
// this is just a UINT8_t with below value

#define GW_TL_COLOR_OFF 	0x00
#define GW_TL_COLOR_RED 	0x01
#define GW_TL_COLOR_GREEN 	0x02
#define GW_TL_COLOR_AMBER 	0x04
#define GW_TL_FLASH_TRUE	0x10
#define GW_TL_FLASH_FALSE   0x00

#define CMD_GW_GET_DATA 0xA527
/* this one takes no data
 * but returns the state of basically everything
 */

#define GD_SB_IN_DSW 			0x00000001	/* Door Switch */
#define GD_SB_IN_DL  			0x00000002 /* Door LOCK Sensor */
#define GD_SB_IN_12V_PWR		0x00000004 /* MOD PWR Good Signal */
#define GD_SB_IN_24V_PWR		0x00000008 /* MOD PWR Good Signal */
#define GD_SB_IN_PMP_PWR		0x00000010 /* Pump PWR Good Signal */
#define GD_SB_OUT_DL			0x00000020 /* Door Lock Solenoid Signal */
#define GD_SB_PWR_SPLY_FAN_FAIL	0x00000040 /* Door Lock Solenoid Signal */
#define GD_SB_IN_INT_LIGHTS		0x00000080 /* Interior Light On */

#define RESP_GW_CODE_OK 0xA501
#define RESP_GW_CODE_ERROR 0xA5FF
#define RESP_GW_CODE_UNIMPLEMENTED 0xA5EE

//Following this will be a 32 bit length, then data associated?
struct RESP_GW {
	uint16_t code; //See RESP_GW_CODE_*
	uint16_t command; //Original Command
	uint32_t size;
};

struct __attribute__((packed)) DATA_GATEWAY  {
	uint16_t systemPressure;
	uint16_t pressureThreshHigh;
	uint16_t pressureTheshLow;

	uint16_t systemVacuum;
	uint16_t vacuumThreshHigh;
	uint16_t vacuumTheshLow;

	uint8_t spiInputs;
	uint32_t status;

	uint32_t numModules;
};



#endif /* GATEWAY_H_ */
