/*
 * ProjectCode.c
 *
 *  Created on: Aug 5, 2019
 *      Author: dmartin
 */

#include "FreeRTOS.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP.h"
#include "task.h"
#include "main.h"
#include "rng.h"
#include "gateway.h"

/* Mac Address Setup Stuff */
static uint8_t ucMACAddress[6] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
UBaseType_t xTasksAlreadyCreated = pdFALSE;
/* Define the network addressing.  These parameters will be used if either
 ipconfigUDE_DHCP is 0 or if ipconfigUSE_DHCP is 1 but DHCP auto configuration
 failed. */
static const uint8_t ucIPAddress[4] = { 192, 168, 81, 62 };
static const uint8_t ucNetMask[4] = { 255, 255, 255, 0 };
static const uint8_t ucGatewayAddress[4] = { 192, 168, 81, 1 };
static const uint8_t ucDNSServerAddress[4] = { 192, 168, 81, 2 };

static TaskHandle_t serverSocketTaskHandle;

/**
 * Closes socket and delete the task this is executed from, should only be used from functions called in connection_handle
 */
void close_socket(Socket_t sock) {
	FreeRTOS_closesocket(sock);
	vTaskDelete(NULL);
}

void send_cmd_response(Socket_t client, uint16_t cmd,uint16_t errorcode,void *data, BaseType_t length) {
	BaseType_t bytes = 0;
	struct RESP_GW response;

	response.code = errorcode;
	response.command = cmd;
	response.size = length;

	bytes = FreeRTOS_send(client,&response,sizeof(response),0);

	if(bytes < 0) { close_socket(client); }
	if(length > 0) {
		bytes = FreeRTOS_send(client,data,length,0);
	}
}

void handle_set_pressure(Socket_t client) {
	// read a pressure setting from the socket
	BaseType_t bytes = 0;
	struct DATA_GW_CMD_SET_PRESS pressure = { 0,0 };

	bytes = FreeRTOS_recv(client, &pressure, sizeof(pressure),0);
	if(bytes != sizeof(pressure)) {
		close_socket(client);
	} else {
		send_cmd_response(client,CMD_GW_SET_SYS_PRESSURE,RESP_GW_CODE_OK,NULL,0);
	}
}

void handle_set_vacuum(Socket_t client) {
	// read a pressure setting from the socket
		BaseType_t bytes = 0;
		struct DATA_GW_CMD_SET_PRESS pressure = { 0,0 };

		bytes = FreeRTOS_recv(client, &pressure, sizeof(pressure),0);
		if(bytes != sizeof(pressure)) {
			close_socket(client);
		} else {
			send_cmd_response(client,CMD_GW_SET_SYS_VACUUM,RESP_GW_CODE_OK,NULL,0);
		}
}

void handle_set_toplight(Socket_t client) {
	// read a pressure setting from the socket
		BaseType_t bytes = 0;
		uint8_t data;

		bytes = FreeRTOS_recv(client, &data, sizeof(data),0);
		if(bytes != sizeof(data)) {
			close_socket(client);
		} else {
			send_cmd_response(client,CMD_GW_SET_TOPLIGHT,RESP_GW_CODE_OK,NULL,0);
		}
}

void handle_get_data_packet(Socket_t client) {
	//send out the system state packet


	float fp1 = 0.5411f;
	float fp2 = 0.6531f;
	float fp3 = fp1 * fp2;


	struct DATA_GATEWAY datapacket;

	datapacket.numModules = 2;
	datapacket.pressureTheshLow = 42;
	datapacket.pressureThreshHigh = 84;
	datapacket.spiInputs = 0;
	datapacket.status = 0;
	datapacket.systemPressure = 54;
	datapacket.systemVacuum = 21;
	datapacket.vacuumTheshLow = 10;
	datapacket.vacuumThreshHigh = 42;

	send_cmd_response(client,CMD_GW_GET_DATA,RESP_GW_CODE_OK,&datapacket,sizeof(datapacket));
}

/* Connection handler Task, one of these for each connection */
void connection_handle(void * params) {
	Socket_t *client_ptr = (Socket_t * )params;
	Socket_t client = *client_ptr;
	BaseType_t bytesRecv;
	uint8_t scratch;
	uint16_t cmd;

	//Maybe spit out short blurb
	for(;;) {
		bytesRecv = FreeRTOS_recv(client, &cmd, 2, 0);

		if(bytesRecv < 0) {
			if(!FreeRTOS_issocketconnected(client)) {
				FreeRTOS_closesocket(client);
			} else {
				FreeRTOS_shutdown(client, FREERTOS_SHUT_RDWR);
				bytesRecv = 42;
				while(bytesRecv > 0) {
					bytesRecv = FreeRTOS_recv(client,&scratch,1,0);
				}
			}
			vTaskDelete(NULL);
			vTaskDelay(1000);
			for(;;) { }
		}

		switch(cmd) {
			case CMD_GW_SET_SYS_PRESSURE:
				handle_set_pressure(client);
				break;
			case CMD_GW_SET_SYS_VACUUM:
				handle_set_vacuum(client);
				break;
			case CMD_GW_SET_TOPLIGHT:
				handle_set_toplight(client);
				break;
			case CMD_GW_GET_DATA:
				handle_get_data_packet(client);
				break;
			default:
				send_cmd_response(client,cmd,RESP_GW_CODE_UNIMPLEMENTED,NULL,0);
				break;
		}
		vTaskDelay(5);
	}
}

/* needed by FreeRTOS+TCP for sequence number generation, using hardware RNG */
UBaseType_t uxRand() {
	uint32_t random32bit;
	HAL_StatusTypeDef status;
	status = HAL_RNG_GenerateRandomNumber(&hrng, &random32bit);
	configASSERT(status == HAL_OK);
	return random32bit;
}


/* Listening task, Listen on port 4242 for incomming connections and spawn a ConnectionHandler */
void ListenTask(void *params) {
	Socket_t serversock;
	Socket_t clientsock;
	struct freertos_sockaddr xBindAddress, xClientAddress;
	uint8_t data;
	BaseType_t bytesRecv = 0;
	char *message = "HELLO WORLD!\n";
	size_t len = 13;

	static const TickType_t xReceiveTimeOut = portMAX_DELAY;

	socklen_t pxAddressLength = sizeof(xClientAddress);

	serversock = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM,
	FREERTOS_IPPROTO_TCP);

	FreeRTOS_setsockopt( serversock,
	                         0,
	                         FREERTOS_SO_RCVTIMEO,
	                         &xReceiveTimeOut,
	                         sizeof( xReceiveTimeOut ) );

	configASSERT(serversock != FREERTOS_INVALID_SOCKET);
	xBindAddress.sin_port = FreeRTOS_htons(4242);
	FreeRTOS_bind(serversock, &xBindAddress, sizeof(xBindAddress));
	FreeRTOS_listen(serversock, 2);

	for (;;) {
		clientsock = FreeRTOS_accept(serversock, &xClientAddress,
				&pxAddressLength);

			configASSERT(clientsock != FREERTOS_INVALID_SOCKET);

			FreeRTOS_send(clientsock,message,len,0);

			xTaskCreate(connection_handle, "ClientTask", 128, &clientsock, tskIDLE_PRIORITY,NULL);

	}

}

/* Code hook for net up and net down */
void vApplicationIPNetworkEventHook(eIPCallbackEvent_t eNetworkEvent) {
	/* Both eNetworkUp and eNetworkDown events can be processed here. */
	if (eNetworkEvent == eNetworkUp) {

		/* Create the tasks that use the TCP/IP stack if they have not already
		 been created. */
		if (xTasksAlreadyCreated == pdFALSE) {

			xTaskCreate(ListenTask, "ServerTask", 128, NULL, tskIDLE_PRIORITY,&serverSocketTaskHandle);

			xTasksAlreadyCreated = pdTRUE;
		}
	} else {
		//vTaskDelete(serverSocketTaskHandle);
		//xTasksAlreadyCreated = pdFALSE;
	}
}

/* Blinky an LED on the board */
void heartbeat () {
	for(;;) {
		vTaskDelay(100);
		HAL_GPIO_TogglePin(LED3_GPIO_Port,LED3_Pin);
	}
}


/* Init FreeRTOS stuff, cubeIDE is picky and if you have something it doesn't like it will delete all your source */
void DoFreeRTOS() {
	FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress,ucDNSServerAddress, ucMACAddress);
	xTaskCreate(heartbeat,"Heartbeat",128,NULL,0,NULL);
	vTaskStartScheduler();

}
