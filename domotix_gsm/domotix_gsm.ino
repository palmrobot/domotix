#include <GSM.h>

/********************************************************/
/*      State  GSM definitions                          */
/********************************************************/


#define COMMAND_START			0x21
#define COMMAND_REPLY			0x22

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
    |-- FA 83 XX "message to send" YY -> | Message to ask if GSM is init
    |<--------- FB YY--------------------| Ack with CRC from sent data

    |                                    |
*/

#define IO_GSM_COMMAND_SMS			0x30
#define IO_GSM_COMMAND_IS_INIT			0x31

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

#define GSM_IO_COMMAND_INIT_OK			0x41
#define GSM_IO_COMMAND_INIT_FAILED		0x42
#define GSM_IO_COMMAND_LIGHT_1			0x43
#define GSM_IO_COMMAND_CRITICAL_TIME		0x44

#define CMD_DATA_MAX				60

uint8_t g_recv_masterIO[CMD_DATA_MAX];
uint8_t g_send_to_masterIO[CMD_DATA_MAX];
uint8_t g_command = 0;


#define CMD_STATE_START				1
#define CMD_STATE_CMD				2
#define CMD_STATE_SIZE				3
#define CMD_STATE_DATA				4
#define CMD_STATE_CRC				5
#define CMD_STATE_END				6

uint8_t g_recv_state;
uint8_t g_recv_size;
uint8_t g_recv_index;
uint8_t g_recv_crc;

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
#define SMS_RESP_4				"Alerte"

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

char    g_sms_recv_buffer[CMD_DATA_MAX];
char    g_sms_send_buffer[CMD_DATA_MAX];


/********************************************************/
/*      Process definitions                             */
/********************************************************/


uint8_t g_process_recv_masterIO;
#define PROCESS_RECV_MASTERIO_DO_NOTHING		0
#define PROCESS_RECV_MASTERIO_WAIT_COMMAND		1

uint8_t g_process_action;
#define PROCESS_ACTION_NONE				0
#define PROCESS_ACTION_SMS				IO_GSM_COMMAND_SMS
#define PROCESS_ACTION_IS_INIT				IO_GSM_COMMAND_IS_INIT
#define PROCESS_ACTION_TEST				IO_GSM_COMMAND_TEST

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
    g_recv_state		= CMD_STATE_START;
    g_recv_size			= 0;
    g_recv_index		= 0;

    /* init pipes */
    g_recv_masterIO[0]	= 0;

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

    /* initialize serial communications at 115200 bps: */
    Serial.begin(115200);
    delay(100);

    /* Then send OK is ready */
    send_masterIO(GSM_IO_COMMAND_INIT_OK, NULL, 0);

    sprintf(g_sms_send_buffer, SMS_RESP_0 );
    send_SMS(g_sms_send_buffer, NICO_NUMBER, 1);

    g_gsm_init = GSM_INIT_OK;

    /* start polling sms */
    g_process_recv_sms = PROCESS_RECV_SMS_WAIT_SMS;
}


void send_masterIO(uint8_t cmd, uint8_t *buffer, uint8_t size)
{
    uint8_t crc = 42;

    if (size > CMD_DATA_MAX)
	size = CMD_DATA_MAX;

    /* Send Start of transmission */
    Serial.write(COMMAND_START);

    /* Send Command  */
    Serial.write(cmd);

    /* Send Size of data */
    Serial.write(size);

    /* Write Data */
    if ((buffer != NULL) && (size > 0))
    {
	Serial.write(buffer, size);
    }

    /* Send CRC */
    Serial.write(crc);
}


void send_SMS(char *message, const char *phone_number, uint8_t force)
{
    if ((g_critical_alertes == GSM_CRITICAL_ALERTE_ON) || (force == 1))
    {
	/* send message */
	sprintf(g_sender_number, phone_number);
	g_gsm_sms.beginSMS(g_sender_number);
	if (strlen(message) > (CMD_DATA_MAX-1))
	{
	    /* then trunc it */
	    message[CMD_DATA_MAX-1] = 0;
	}
	g_gsm_sms.print(message);
	g_gsm_sms.endSMS();
	delay(500);
    }
}


void process_recv_masterIO(void)
{
    uint8_t value;
    uint8_t i;
    uint8_t crc;
    uint8_t recv_crc;

    if (g_process_recv_masterIO == PROCESS_RECV_MASTERIO_WAIT_COMMAND)
    {
	switch(g_recv_state)
	{
	    case CMD_STATE_START:
	    {
		/* if we get a valid char, read char */
		if (Serial.available() > 0)
		{
		    value = Serial.read();

		    if (value == COMMAND_START)
		    {
			/* next state */
			g_recv_state = CMD_STATE_CMD;
		    }
		    else
		    {
			/* retry state */
			g_recv_state = CMD_STATE_START;
		    }
		}
	    }break;
	    case CMD_STATE_CMD:
	    {
		/* if we get a valid char, read char */
		if (Serial.available() > 0)
		{
		    g_command = Serial.read();

		    /* next state */
		    g_recv_state = CMD_STATE_SIZE;
		}
	    }break;
	    case CMD_STATE_SIZE:
	    {
		/* if we get a valid char, read char */
		if (Serial.available() > 0)
		{
		    g_recv_size = Serial.read();

		    if (g_recv_size > 0)
		    {
			g_recv_index = 0;
			g_recv_crc   = 0;

			/* next state */
			g_recv_state = CMD_STATE_DATA;
		    }
		    else
		    {
			/* next state */
			g_recv_state = CMD_STATE_CRC;
		    }
		}
	    }break;
	    case CMD_STATE_DATA:
	    {
		/* if we get a valid char, read char */
		if (Serial.available() > 0)
		{
		    /* get incoming data: */
		    g_recv_masterIO[g_recv_index] = Serial.read();
		    g_recv_crc += g_recv_masterIO[g_recv_index];
		    g_recv_index++;

		    if (g_recv_index == g_recv_size)
		    {
			/* Set null terminated string */
			g_recv_masterIO[g_recv_index] = 0;

			/* next state */
			g_recv_state = CMD_STATE_CRC;
		    }
		}
	    }break;
	    case CMD_STATE_CRC:
	    {
		/* if we get a valid char, read char */
		if (Serial.available() > 0)
		{
		    recv_crc  = Serial.read();

		    /* next state */
		    g_recv_state = CMD_STATE_END;

		    /* =======================> BYPASS CRC !!!!!! */
#if 0
		    /* check CRC */
		    if (recv_crc != g_recv_crc)
		    {
			/* wrong command
			 * Discard it
			 */
			g_command = 0;

			/* start waiting for an other comand */
			g_process_recv_masterIO = PROCESS_RECV_MASTERIO_WAIT_COMMAND;

			/* restart */
			g_recv_state = CMD_STATE_START;
		    }
		    else
		    {
			/* next state */
			g_recv_state = CMD_STATE_END;
		    }
#endif
		}
	    }break;
	    case CMD_STATE_END:
	    {
		/* Set action plan */
		g_process_action = g_command;

		/* Disable communication ,wait for message treatment */
		g_process_recv_masterIO = PROCESS_RECV_MASTERIO_DO_NOTHING;

		/* next state */
		g_recv_state = CMD_STATE_START;
	    }break;
	    default:
	    {
	    }
	    break;
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
		g_sms_recv_buffer[i] = g_gsm_sms.read();
		i++;
	    }
	    while (g_sms_recv_buffer[i-1] != 0);

	    /* Check sender */
	    if (strstr(g_sender_number, NICO_NUMBER) != NULL)
	    {
		if (strcmp(g_sms_recv_buffer, SMS_RECV_1) == 0)
		{
		    sprintf(g_sms_send_buffer,SMS_RESP_1_N);
		    send_SMS(g_sms_send_buffer, NICO_NUMBER, 1);
		}
		else if (strcmp(g_sms_recv_buffer, SMS_RECV_2) == 0)
		{
		    sprintf(g_sms_send_buffer, SMS_RESP_2);
		    send_SMS(g_sms_send_buffer, NICO_NUMBER, 1);

		    g_critical_alertes = GSM_CRITICAL_ALERTE_OFF;

		    g_send_to_masterIO[0] = 0;
		    send_masterIO(GSM_IO_COMMAND_CRITICAL_TIME, g_send_to_masterIO, 1);
		}
		else if (strcmp(g_sms_recv_buffer, SMS_RECV_3) == 0)
		{
		    sprintf(g_sms_send_buffer, SMS_RESP_3);
		    send_SMS(g_sms_send_buffer, NICO_NUMBER, 1);
		    g_critical_alertes = GSM_CRITICAL_ALERTE_ON;

		    g_send_to_masterIO[0] = 1;
		    send_masterIO(GSM_IO_COMMAND_CRITICAL_TIME, g_send_to_masterIO, 1);
		}
		else if (strcmp(g_sms_recv_buffer, SMS_RECV_4) == 0)
		{
		    g_send_to_masterIO[0] = 1;
		    send_masterIO(GSM_IO_COMMAND_LIGHT_1, g_send_to_masterIO, 1);
		}
		else if (strcmp(g_sms_recv_buffer, SMS_RECV_5) == 0)
		{
		    g_send_to_masterIO[0] = 0;
		    send_masterIO(GSM_IO_COMMAND_LIGHT_1, g_send_to_masterIO, 1);
		}
	    }
	    else if (strstr(g_sender_number, ESTELLE_NUMBER) != NULL)
	    {
		if (strcmp(g_sms_recv_buffer, SMS_RECV_1) == 0)
		{
		    sprintf(g_sms_send_buffer,SMS_RESP_1_E );
		    send_SMS(g_sms_send_buffer, ESTELLE_NUMBER, 1);
		}
	    }

	    /* Discard */
	    delay(500);

	    /* Delete message from modem memory */
	    g_gsm_sms.flush();
	}
    }
}

void process_action(void)
{
    uint8_t action_done;

    if (g_process_action != PROCESS_ACTION_NONE)
    {
	action_done = 1;

	switch(g_process_action)
	{
	    case PROCESS_ACTION_SMS:
	    {
		send_SMS((char*)g_recv_masterIO, NICO_NUMBER, 0);
	    }break;
	    case PROCESS_ACTION_IS_INIT:
	    {
		if (g_gsm_init == GSM_INIT_OK)
		{
		    /*  send OK is ready */
		    send_masterIO(GSM_IO_COMMAND_INIT_OK, NULL, 0);
		}
	    }break;
	    default:
	    {
	    }
	    break;
	}

	if (action_done)
	{
	    /* start waiting for an other comand */
	    g_process_recv_masterIO = PROCESS_RECV_MASTERIO_WAIT_COMMAND;

	    /* Reset action, wait for next one */
	    g_process_action = PROCESS_ACTION_NONE;
	}
    }
}

void loop()
{
    process_recv_masterIO();
    process_action();
    process_recv_gsm_sms();
}


