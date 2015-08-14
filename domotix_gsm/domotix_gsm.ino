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
    |-- FA 82 00 00 -------------------->| Ask for init GSM module
    |<--------- FB 00--------------------| Ack with CRC from sent data
    |                                    |
    |-- FA 83 XX "message to send" YY -> | Message to send by SMS
    |<--------- FB YY--------------------| Ack with CRC from sent data
    |                                    |
*/

#define IO_GSM_COMMAND_INIT			0x82
#define IO_GSM_COMMAND_SMS			0x83


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

uint8_t g_recv_masterIO[CMD_DATA_MAX];
uint8_t g_send_to_masterIO[CMD_DATA_MAX];
uint8_t g_gsm_command = 0;

/********************************************************/
/*      Key  definitions                                */
/********************************************************/
/* 1) "xxxxx Domotix"  => "Bonjour Name"
 *
 * 2) "
 */

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
char    g_sms_buffer[150];


/********************************************************/
/*      Process definitions                             */
/********************************************************/


uint8_t g_process_recv_masterIO;
#define PROCESS_RECV_MASTERIO_DO_NOTHING		0
#define PROCESS_RECV_MASTERIO_WAIT_COMMAND		1

uint8_t g_process_action;
#define PROCESS_ACTION_NONE				0
#define PROCESS_ACTION_INIT				IO_GSM_COMMAND_INIT
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


void setup()
{
    /* Initialize the output pins for the DAC control. */


    /* init process states */
    g_process_recv_masterIO	= PROCESS_RECV_MASTERIO_WAIT_COMMAND;
    g_process_action		= PROCESS_ACTION_NONE;
    g_process_recv_sms		= PROCESS_RECV_SMS_DO_NOTHING;

    /* Init global variables */
    g_gsm_init			 = GSM_INIT_FAILED;

    /* init pipes */
    g_recv_masterIO[0]	= 0;

    /* initialize serial communications at 115200 bps: */
    Serial.begin(115200);
    delay(100);
}


void send_masterIO(uint8_t cmd, uint8_t *buffer, uint8_t size)
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
	Serial.write(buffer, size);
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

	    }
	    else if (strstr(g_sender_number, ESTELLE_NUMBER) != NULL)
	    {

	    }
	    else
	    {
		/* Discard */
		/* send acces denied  message */
		g_gsm_sms.beginSMS(g_sender_number);
		sprintf(g_sms_buffer,"! Acces Denied !");
		g_gsm_sms.print(g_sms_buffer);
		g_gsm_sms.endSMS();
		delay(500);
	    }
	}
    }
}

void process_action(void)
{
    boolean not_connected;

    if (g_process_action)
    {
	if (g_process_action == PROCESS_ACTION_INIT)
	{
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
	    send_masterIO(GSM_IO_COMMAND_INIT_OK, NULL, 1);

	    g_gsm_init = GSM_INIT_OK;

	    /* start polling sms */
	    g_process_recv_sms = PROCESS_RECV_SMS_WAIT_SMS;

	    /* restart waiting for command */
	    g_process_recv_masterIO == PROCESS_RECV_MASTERIO_WAIT_COMMAND;

	}
	else if (g_process_action == PROCESS_ACTION_SMS)
	{
	    /* send message */
	    sprintf(g_sender_number, NICO_NUMBER);
	    g_gsm_sms.beginSMS(g_sender_number);
	    g_gsm_sms.print((char*)g_recv_masterIO);
	    g_gsm_sms.endSMS();
	    delay(500);
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


