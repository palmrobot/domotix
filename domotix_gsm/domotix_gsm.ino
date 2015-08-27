#include <GSM.h>

/********************************************************/
/*      State  GSM definitions                          */
/********************************************************/


#define COMMAND_START			0xFA
#define COMMAND_REPLY			0xFB

/*
Master I/O Board                     GSM Board
    |                                    |
    |                                    |
    |--START CMD NB_DATA DATA CRC------->|
    |<----------ACK CRC------------------|
    |                                    |
    |                                    |
    |-- FA 82 XX "message to send" YY -> | Message to send by SMS
    |<--------- FB YY--------------------| Ack with CRC from sent data
    |                                    |
*/

#define IO_GSM_COMMAND_SMS			0x82


/*
Master I/O Board                     GSM Board
    |                                    |
    |                                    |
    |--START CMD NB_DATA DATA CRC------->|
    |<----------ACK CRC------------------|
    |                                    |
    |<--FA D1 00 CC ---------------------| Init GSM module OK
    |--------- FB CC-------------------->| Ack with CRC from sent data
    |                                    |
    |<--FA D2 00 CC ---------------------| Init GSM module FAILED
    |--------- FB CC-------------------->| Ack with CRC from sent data
    |                                    |
    |<--FA D3 01 XX CC ------------------| Switch ON/OFF light 1
    |--------- FB CC-------------------->| Ack with CRC from sent data
    |                                    |
    |<--FA D4 01 XX CC ------------------| Critical time enable/disable
    |--------- FB CC-------------------->| Ack with CRC from sent data
    |                                    |

*/

#define GSM_IO_COMMAND_INIT_OK			0xD1
#define GSM_IO_COMMAND_INIT_FAILED		0xD2
#define GSM_IO_COMMAND_LIGHT_1			0xD3
#define GSM_IO_COMMAND_CRITICAL_TIME		0xD4


#define CMD_DATA_MAX				60

char g_recv_masterIO[CMD_DATA_MAX];
char g_send_to_masterIO[CMD_DATA_MAX];
char g_gsm_command = 0;

/********************************************************/
/*      Key  definitions                                */
/********************************************************/
/* 1) "Bonjour Domotix"  => "Bonjour Name !"
 *
 * 2) "Stop les alertes" => "Les alertes sont desactivees"
 *
 * 3) "Demarre les alertes" => "Les alertes sont activees"
 */

#define SMS_RESP_0				"Systeme d'envoi de SMS ready !!"

#define SMS_RECV_1				"Bonjour Domotix"
#define SMS_RESP_1_N				"Bonjour Nicolas !!"
#define SMS_RESP_1_E				"Bonjour Estelle !!"

#define SMS_RECV_2				"Stop les alertes"
#define SMS_RESP_2				"Les envois d'alertes sont desactivees"

#define SMS_RECV_3				"Demarre les alertes"
#define SMS_RESP_3				"Les alertes sont activees"

#define SMS_RECV_4				"Allume la lumiere 1"
#define SMS_RECV_5				"Eteint la lumiere 1"

/********************************************************/
/*      Pin  definitions                                */
/********************************************************/



/********************************************************/
/*      GSM  definitions                                */
/********************************************************/
/* PIN Number for the SIM */
#define PIN_SIM_CODE				"2153"
#define PHONE_NUMBER				""
#define NICO_NUMBER				"+33668089948"
#define ESTELLE_NUMBER				"+33668088058"

/* initialize the library instances */
GSM	g_gsm_access;
GSM_SMS g_gsm_sms;

/* Array to hold the number a SMS is retreived from */
#define PHONE_NUMBER_LEN			20
char    g_sender_number[PHONE_NUMBER_LEN];

#define SMS_LEN					150
char    g_sms_buffer[SMS_LEN+1];


/********************************************************/
/*      Process definitions                             */
/********************************************************/


uint8_t g_process_recv_masterIO;
#define PROCESS_RECV_MASTERIO_DO_NOTHING		0
#define PROCESS_RECV_MASTERIO_WAIT_COMMAND		1

uint8_t g_process_action;
#define PROCESS_ACTION_NONE				0
#define PROCESS_ACTION_SMS				IO_GSM_COMMAND_SMS

uint8_t g_process_recv_sms;
#define PROCESS_RECV_SMS_DO_NOTHING			0
#define PROCESS_RECV_SMS_WAIT_SMS			1

/********************************************************/
/*      Global definitions                              */
/********************************************************/

#define GSM_INIT_FAILED					0
#define GSM_INIT_OK					1
uint8_t g_gsm_init;

#define GSM_CRITICAL_ALERTE_OFF				0
#define GSM_CRITICAL_ALERTE_ON				1
uint8_t g_critical_alertes;

void setup()
{
    boolean not_connected;

    /* Initialize the output pins for the DAC control. */


    /* init process states */
    g_process_recv_masterIO	= PROCESS_RECV_MASTERIO_WAIT_COMMAND;
    g_process_action		= PROCESS_ACTION_NONE;
    g_process_recv_sms		= PROCESS_RECV_SMS_DO_NOTHING;

    /* Init global variables */
    g_gsm_init			= GSM_INIT_FAILED;
    g_critical_alertes		= GSM_CRITICAL_ALERTE_OFF;

    /* init pipes */
    g_recv_masterIO[0]	= 0;

    /* initialize serial communications at 115200 bps: */
    Serial.begin(115200);
    delay(100);

    /* init GSM */
    /* Start GSM connection */
    not_connected = true;

    while(not_connected)
    {
	if(g_gsm_access.begin(PIN_SIM_CODE) == GSM_READY)
	{
	    not_connected = false;
	}
	else
	{
	    delay(500);
	}
    }

    /* Then send OK is ready */
    send_masterIO(GSM_IO_COMMAND_INIT_OK, NULL, 0);

    g_critical_alertes = GSM_CRITICAL_ALERTE_ON;
    send_SMS(SMS_RESP_0, NICO_NUMBER);
    g_critical_alertes = GSM_CRITICAL_ALERTE_OFF;

    g_gsm_init = GSM_INIT_OK;

    /* start polling sms */
    g_process_recv_sms = PROCESS_RECV_SMS_WAIT_SMS;
}


void send_masterIO(uint8_t cmd, char *buffer, uint8_t size)
{
    if (size > CMD_DATA_MAX)
	size = CMD_DATA_MAX;

    /* Send Start of transmission */
    Serial.write(COMMAND_START);

    /* Send Command  */
    Serial.write(cmd);

    /* Send Size of data */
    Serial.write(size);

    /* Write Data */
    if (buffer != NULL)
    {
	Serial.write((unsigned char*)buffer, size);
    }
}


void send_SMS(char *message, const char *phone_number)
{
    if (g_critical_alertes == GSM_CRITICAL_ALERTE_ON)
    {
	/* send message */
	sprintf(g_sender_number, phone_number);
	g_gsm_sms.beginSMS(g_sender_number);
	if (strlen(message) > SMS_LEN)
	{
	    /* then tronc it */
	    message[SMS_LEN] = '\0';
	}
	g_gsm_sms.print((char*)message);
	g_gsm_sms.endSMS();
	delay(500);
    }
}


void process_recv_masterIO(void)
{
    uint8_t value;
    uint8_t size;
    uint8_t i;
    uint8_t crc;
    uint8_t recv_crc;

    if (g_process_recv_masterIO == PROCESS_RECV_MASTERIO_WAIT_COMMAND)
    {
	/* if we get a valid char, read char */
	if (Serial.available() > 0)
	{
	    /* get Start Byte */
	    value = Serial.read();
	    if (value == COMMAND_START)
	    {
		/* Read Command */
		while (Serial.available() == 0);
		g_gsm_command = Serial.read();

		/* Read Size of Data */
		while (Serial.available() == 0);
		size = Serial.read();

		/* Wait for all data */
		while (Serial.available() < size);

		crc = 0;
		for(i = 0; i < size; i++)
		{
		    /* get incoming data: */
		    g_recv_masterIO[i] = Serial.read();
		    crc += g_recv_masterIO[i];
		}

		/* Add null terminated string */
		g_recv_masterIO[size-1] = '\0';

		/* Read CRC of Data */
		while (Serial.available() == 0);
		recv_crc = Serial.read();

		/* check CRC */
		if (recv_crc != crc)
		{
		    /* wrong command
		     * Discard it
		     */
		    g_gsm_command = 0;
		}

		/* Set action plan */
		g_process_action = g_gsm_command;

		/* Disable communication ,wait for message treatment */
		g_process_recv_masterIO = PROCESS_RECV_MASTERIO_DO_NOTHING;
	    }
	}
    }
}


void process_recv_gsm_sms(void)
{
    uint8_t i;

    if (g_process_recv_sms)
    {
	/* check if a message has been received */
	if (g_gsm_sms.available())
	{
	    /* Get remote number */
	    g_gsm_sms.remoteNumber(g_sender_number, PHONE_NUMBER_LEN);

	    /* Read message bytes and print them */
	    i = 0;
	    do
	    {
		g_sms_buffer[i] = g_gsm_sms.read();
		i++;
	    }
	    while (g_sms_buffer[i-1] != 0);

	    /* Delete message from modem memory */
	    g_gsm_sms.flush();

	    /* Check sender */
	    if (strstr(g_sender_number, NICO_NUMBER) != NULL)
	    {
		if (strcmp(g_sms_buffer, SMS_RECV_1) == 0)
		{
		    send_SMS(SMS_RESP_1_N, NICO_NUMBER);
		}
		else if (strcmp(g_sms_buffer, SMS_RECV_2) == 0)
		{
		    send_SMS(SMS_RESP_2, NICO_NUMBER);
		    g_critical_alertes = GSM_CRITICAL_ALERTE_OFF;

		    g_send_to_masterIO[0] = 0;
		    send_masterIO(GSM_IO_COMMAND_CRITICAL_TIME, g_send_to_masterIO, 1);
		}
		else if (strcmp(g_sms_buffer, SMS_RECV_3) == 0)
		{
		    send_SMS(SMS_RESP_3, NICO_NUMBER);
		    g_critical_alertes = GSM_CRITICAL_ALERTE_ON;

		    g_send_to_masterIO[0] = 1;
		    send_masterIO(GSM_IO_COMMAND_CRITICAL_TIME, g_send_to_masterIO, 1);
		}
	    }
	    else if (strstr(g_sender_number, ESTELLE_NUMBER) != NULL)
	    {
		if (strcmp(g_sms_buffer, SMS_RECV_1) == 0)
		{
		    send_SMS(SMS_RESP_1_E, ESTELLE_NUMBER);
		}
	    }
	    else if (strstr(g_sender_number, ESTELLE_NUMBER) != NULL)
	    {
		if (strcmp(g_sms_buffer, SMS_RECV_4) == 0)
		{
		    g_send_to_masterIO[0] = 1;
		    send_masterIO(GSM_IO_COMMAND_LIGHT_1, g_send_to_masterIO, 1);
		}
	    }
	    else if (strstr(g_sender_number, ESTELLE_NUMBER) != NULL)
	    {
		if (strcmp(g_sms_buffer, SMS_RECV_5) == 0)
		{
		    g_send_to_masterIO[0] = 0;
		    send_masterIO(GSM_IO_COMMAND_LIGHT_1, g_send_to_masterIO, 1);
		}
	    }

	    else
	    {
		/* Discard */
		delay(500);
	    }
	}
    }
}

void process_action(void)
{
    if (g_process_action)
    {
	if (g_process_action == PROCESS_ACTION_SMS)
	{
	    send_SMS(g_recv_masterIO, NICO_NUMBER);
	}

	/* end of action, wait for a new one */
	g_process_action = PROCESS_ACTION_NONE;
    }
}

void loop()
{
    process_recv_masterIO();
    process_action();
    process_recv_gsm_sms();
}


