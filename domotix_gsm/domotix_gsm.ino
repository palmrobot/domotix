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
    |--   START CMD NB_DATA DATA ------->|
    |                                    |
    |--   FA 82 XX "message to send"  -> | Message to send by SMS
    |                                    |
    |--   FA 83 XX "message to send"  -> | Message to ask if GSM is init
    |                                    |
*/

#define IO_GSM_COMMAND_SMS			0x30

/*
Master I/O Board                     GSM Board
    |                                    |
    |                                    |
    |--START CMD NB_DATA DATA ---------->|
    |                                    |
    |<--FA D1 00 ------------------------| Init GSM module OK
    |                                    |
    |<--FA D2 00 ------------------------| Init GSM module FAILED
    |                                    |
    |<--FA D3 01 XX ---------------------| Switch ON/OFF light 1
    |                                    |
    |<--FA D4 01 XX ---------------------| Critical time enable/disable
    |                                    |

*/

#define GSM_IO_COMMAND_INIT_FAILED		0x42
#define GSM_IO_COMMAND_LIGHT_1			0x43
#define GSM_IO_COMMAND_CRITICAL_TIME		0x44

#define CMD_DATA_MAX				60
#define MIN_CMD_SIZE				3

uint8_t g_recv_masterIO[CMD_DATA_MAX];
uint8_t g_send_to_masterIO[CMD_DATA_MAX];

#define MAX_WAIT_SERIAL				500
#define TIME_WAIT_SERIAL			10

/********************************************************/
/*      Pin  definitions                                */
/********************************************************/
#define PIN_OUT_MASTER				4 /* init GSM to master  */


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
#define PROCESS_ACTION_TEST				IO_GSM_COMMAND_TEST

uint8_t g_process_recv_sms;
#define PROCESS_RECV_SMS_DO_NOTHING			0
#define PROCESS_RECV_SMS_WAIT_SMS			1

/********************************************************/
/*      Global definitions                              */
/********************************************************/

#define LAMPE_OFF					1
#define LAMPE_ON					0

#define GSM_INIT_FAILED					0
#define GSM_INIT_OK					1
uint8_t g_gsm_init;

void setup()
{
    boolean not_connected;

    pinMode(PIN_OUT_MASTER, OUTPUT);

    digitalWrite(PIN_OUT_MASTER, 0);

    /* init process states */
    g_process_recv_masterIO	= PROCESS_RECV_MASTERIO_WAIT_COMMAND;
    g_process_action		= PROCESS_ACTION_NONE;
    g_process_recv_sms		= PROCESS_RECV_SMS_DO_NOTHING;

    /* Init global variables */
    g_gsm_init			= GSM_INIT_FAILED;

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

    snprintf(g_sms_send_buffer, CMD_DATA_MAX, "Systeme d'envoi de SMS ready !");
    send_SMS(g_sms_send_buffer, NICO_NUMBER);

    g_gsm_init = GSM_INIT_OK;
    digitalWrite(PIN_OUT_MASTER, 1);

    /* start polling sms */
    g_process_recv_sms = PROCESS_RECV_SMS_WAIT_SMS;
}


void send_msg_to_masterIO(uint8_t cmd, uint8_t *buffer, uint8_t size)
{
    if (size > CMD_DATA_MAX)
	size = CMD_DATA_MAX;

    /* Send byte of Start of transmission */
    Serial.write(COMMAND_START);

    /* Send byte of Command  */
    Serial.write(cmd);

    /* Send byte of Size of data */
    Serial.write(size);

    /* Write Data */
    if ((buffer != NULL) && (size > 0))
    {
	Serial.write(buffer, size);
    }
    Serial.flush();
}

void send_SMS(char *message, const char *phone_number)
{
	/* send message */
	g_gsm_sms.beginSMS(phone_number);
	if (strlen(message) > (CMD_DATA_MAX-1))
	{
	    /* then trunc it */
	    message[CMD_DATA_MAX-1] = 0;
	}
	g_gsm_sms.print(message);
	g_gsm_sms.endSMS();
	delay(500);

	/* Delete message from modem memory */
	g_gsm_sms.flush();
}


void process_recv_masterIO(void)
{
    uint8_t value;
    uint8_t recv_size;
    uint8_t recv_index;
    uint8_t error;
    uint8_t command;
    uint16_t nb_wait;

    if (g_process_recv_masterIO == PROCESS_RECV_MASTERIO_WAIT_COMMAND)
    {
	/* if we get 3 byte, which is the minimum value for cmd */
	if (Serial.available() >= MIN_CMD_SIZE)
	{
	    error = 0;
	    value = Serial.read();
	    if (value == COMMAND_START)
	    {
		/* read command value */
		command = Serial.read();

		/* read size */
		recv_size = Serial.read();
		if (recv_size > 0)
		{
		    recv_index = 0;

		    /* Now read all the data */
		    nb_wait = 0;
		    while ((recv_index < recv_size) && (nb_wait < MAX_WAIT_SERIAL))
		    {
			if (Serial.available() > 0)
			{
			    /* get incoming data: */
			    g_recv_masterIO[recv_index] = Serial.read();
			    recv_index++;
			}
			else
			{
			    nb_wait++;
			}
		    }

		    if (nb_wait == MAX_WAIT_SERIAL)
		    {
			error = 1;
		    }
		    else
		    {
			if (recv_index == recv_size)
			{
			    /* Set null terminated string */
			    g_recv_masterIO[recv_index] = 0;

			    /* Set action plan */
			    g_process_action = command;

			    /* Disable communication ,wait for message treatment */
			    g_process_recv_masterIO = PROCESS_RECV_MASTERIO_DO_NOTHING;
			}
		    }
		}
	    }
	    else
	    {
		error = 1;
	    }

	    if (error == 1)
	    {
		/* read all bytes in serial queue */
		while (Serial.available() > 0 )
		{
		    value = Serial.read();
		}
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

	    /* Check sender */
	    if ((strstr(g_sender_number, NICO_NUMBER) != NULL) || (strstr(g_sender_number, ESTELLE_NUMBER) != NULL))
	    {
		/* Read message bytes and print them */
		i = 0;
		do
		{
		    g_sms_recv_buffer[i] = g_gsm_sms.read();
		    i++;
		}
		while ((g_sms_recv_buffer[i-1] != 0) && (i < CMD_DATA_MAX));

		if (i == CMD_DATA_MAX)
		    g_sms_recv_buffer[i-1] = 0;

		if (strcmp(g_sms_recv_buffer, "Bonjour Domotix") == 0)
		{
		    if (strstr(g_sender_number, NICO_NUMBER) != NULL)
			snprintf(g_sms_send_buffer, CMD_DATA_MAX, "Bonjour Nicolas !!");
		    else
			snprintf(g_sms_send_buffer, CMD_DATA_MAX, "Bonjour Estelle !!");

		    send_SMS(g_sms_send_buffer, g_sender_number);
		}
		else if (strcmp(g_sms_recv_buffer, "Stop les alertes") == 0)
		{
		    g_send_to_masterIO[0] = 0;
		    send_msg_to_masterIO(GSM_IO_COMMAND_CRITICAL_TIME, g_send_to_masterIO, 1);

		    snprintf(g_sms_send_buffer, CMD_DATA_MAX, "Les alertes sont desactivees");
		    send_SMS(g_sms_send_buffer, g_sender_number);
		}
		else if (strcmp(g_sms_recv_buffer, "Demarre les alertes") == 0)
		{
		    g_send_to_masterIO[0] = 1;
		    send_msg_to_masterIO(GSM_IO_COMMAND_CRITICAL_TIME, g_send_to_masterIO, 1);

		    snprintf(g_sms_send_buffer, CMD_DATA_MAX, "Les alertes sont activees");
		    send_SMS(g_sms_send_buffer, g_sender_number);
		}
		else if (strcmp(g_sms_recv_buffer, "Allume la lumiere 1") == 0)
		{
		    g_send_to_masterIO[0] = LAMPE_ON;
		    send_msg_to_masterIO(GSM_IO_COMMAND_LIGHT_1, g_send_to_masterIO, 1);
		}
		else if (strcmp(g_sms_recv_buffer, "Eteint la lumiere 1") == 0)
		{
		    g_send_to_masterIO[0] = LAMPE_OFF;
		    send_msg_to_masterIO(GSM_IO_COMMAND_LIGHT_1, g_send_to_masterIO, 1);
		}
	    }

	    /* Discard */
	    delay(200);

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
		send_SMS((char*)g_recv_masterIO, NICO_NUMBER);
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


