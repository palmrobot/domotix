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

#define GSM_IO_COMMAND_INIT_FAILED		0x42
#define GSM_IO_COMMAND_LIGHT_1			0x43
#define GSM_IO_COMMAND_CRITICAL_TIME		0x44

#define CMD_DATA_MAX				60

uint8_t g_recv_masterIO[CMD_DATA_MAX];
uint8_t g_send_to_masterIO[CMD_DATA_MAX];

#define MAX_WAIT_SERIAL				50
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

#define GSM_INIT_FAILED					0
#define GSM_INIT_OK					1
uint8_t g_gsm_init;

#define GSM_CRITICAL_ALERTE_OFF				0
#define GSM_CRITICAL_ALERTE_ON				1
uint8_t g_critical_alertes;

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
    g_critical_alertes		= GSM_CRITICAL_ALERTE_OFF;

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
    send_SMS(g_sms_send_buffer, NICO_NUMBER, 1);

    g_gsm_init = GSM_INIT_OK;
    digitalWrite(PIN_OUT_MASTER, 1);

    /* start polling sms */
    g_process_recv_sms = PROCESS_RECV_SMS_WAIT_SMS;
}


void send_msg_to_masterIO(uint8_t cmd, uint8_t *buffer, uint8_t size)
{
    uint8_t crc;
    uint8_t recv_crc;
    uint8_t nb_retry;
    uint8_t nb_wait;
    uint8_t i;

    if (size > CMD_DATA_MAX)
	size = CMD_DATA_MAX;

    nb_retry = 3;
    recv_crc = 0;
    do
    {
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
	    Serial.flush();

	    /* Calcul CRC */
	    crc = 0;
	    for(i = 0; i < size ; i ++)
	    {
		crc += buffer[i];
	    }

	    if (crc == 0)
		crc++;
	}
	else
	{
	    /* arbitrary value */
	    crc = 42;
	}

	/* Send byte of CRC */
	Serial.write(crc);

	/* wait for answer */
	nb_wait = 0;
	while ((Serial.available() <= 0) && (nb_wait < MAX_WAIT_SERIAL))
	{
	    delay(TIME_WAIT_SERIAL);
	    nb_wait++;
	}

	if (nb_wait < MAX_WAIT_SERIAL)
	{
	    /* then read crc */
	    recv_crc = Serial.read();

	    nb_retry--;
	}
    }
    while ((crc != recv_crc) && (nb_retry > 0));
}

void send_SMS(char *message, const char *phone_number, uint8_t force)
{
    if ((g_critical_alertes == GSM_CRITICAL_ALERTE_ON) || (force == 1))
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
	delay(500);
    }
}


void process_recv_masterIO(void)
{
    uint8_t value;
    uint8_t i;
    uint8_t crc;
    uint8_t recv_crc;
    uint8_t recv_size;
    uint8_t recv_index;
    uint8_t nb_wait;
    uint8_t error;
    uint8_t command;

    if (g_process_recv_masterIO == PROCESS_RECV_MASTERIO_WAIT_COMMAND)
    {
	/* if we get a valid char, read char */
	if (Serial.available() > 0)
	{
	    error = 0;
	    value = Serial.read();
	    if (value == COMMAND_START)
	    {
		/* wait for command */
		nb_wait = 0;
		while ((Serial.available() <= 0 ) && (nb_wait < MAX_WAIT_SERIAL))
		{
		    delay(TIME_WAIT_SERIAL);
		    nb_wait++;
		}

		if (nb_wait == MAX_WAIT_SERIAL)
		{
		    error = 1;
		}
		else
		{
		    /* read command value */
		    command = Serial.read();

		    /* wait for size */
		    nb_wait = 0;
		    while ((Serial.available() <= 0 ) && (nb_wait < MAX_WAIT_SERIAL))
		    {
			delay(TIME_WAIT_SERIAL);
			nb_wait++;
		    }

		    if (nb_wait == MAX_WAIT_SERIAL)
		    {
			error = 1;
		    }
		    else
		    {
			/* read size */
			recv_size = Serial.read();
			if (recv_size > 0)
			{
			    recv_index = 0;
			    crc   = 0;

			    /* Now read all the data */
			    nb_wait = 0;
			    while ((recv_index < recv_size) && (nb_wait < MAX_WAIT_SERIAL))
			    {
				if (Serial.available() > 0)
				{
				    /* get incoming data: */
				    g_recv_masterIO[recv_index] = Serial.read();
				    crc += g_recv_masterIO[recv_index];
				    recv_index++;
				}
				else
				{
				    delay(TIME_WAIT_SERIAL);
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

				    /* crc == 0 is the error code, then do not use it */
				    if (crc == 0)
					crc++;
				}
				else
				{
				    error = 1;
				}
			    }
			}
			else
			{
			    crc = 42;
			}
		    }

		    if (error == 0)
		    {
			/* wait for CRC */
			nb_wait = 0;
			while ((Serial.available() <= 0 ) && (nb_wait < MAX_WAIT_SERIAL))
			{
			    delay(TIME_WAIT_SERIAL);
			    nb_wait++;
			}
			if (nb_wait == MAX_WAIT_SERIAL)
			{
			    error = 1;
			}
			else
			{
			    /* read CRC */
			    recv_crc  = Serial.read();

			    /* check CRC */
			    if (crc == recv_crc)
			    {
				/* Send CRC */
				Serial.write(crc);

				/* then flush serial */
				Serial.flush();

				/* Set action plan */
				g_process_action = command;

				/* Disable communication ,wait for message treatment */
				g_process_recv_masterIO = PROCESS_RECV_MASTERIO_DO_NOTHING;
			    }
			    else
			    {
				error = 1;
			    }
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
		/* then send CRC error */
		crc = 0;
		Serial.write(crc);

		/* then flush serial */
		Serial.flush();
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
		g_sms_recv_buffer[i] = g_gsm_sms.read();
		i++;
	    }
	    while (g_sms_recv_buffer[i-1] != 0);

	    /* Check sender */
	    if ((strstr(g_sender_number, NICO_NUMBER) != NULL) || (strstr(g_sender_number, ESTELLE_NUMBER) != NULL))
	    {
		if (strcmp(g_sms_recv_buffer, "Bonjour Domotix") == 0)
		{
		    if (strstr(g_sender_number, NICO_NUMBER) != NULL)
			snprintf(g_sms_send_buffer, CMD_DATA_MAX, "Bonjour Nicolas !!");
		    else
			snprintf(g_sms_send_buffer, CMD_DATA_MAX, "Bonjour Estelle !!");

		    send_SMS(g_sms_send_buffer, g_sender_number, 1);
		}
		else if (strcmp(g_sms_recv_buffer, "Stop les alertes") == 0)
		{
		    snprintf(g_sms_send_buffer, CMD_DATA_MAX, "Les envois d'alertes sont desactivees");
		    send_SMS(g_sms_send_buffer, g_sender_number, 1);

		    g_critical_alertes = GSM_CRITICAL_ALERTE_OFF;

		    g_send_to_masterIO[0] = 0;
		    send_msg_to_masterIO(GSM_IO_COMMAND_CRITICAL_TIME, g_send_to_masterIO, 1);
		}
		else if (strcmp(g_sms_recv_buffer, "Demarre les alertes") == 0)
		{
		    snprintf(g_sms_send_buffer, CMD_DATA_MAX, "Les alertes sont activees");
		    send_SMS(g_sms_send_buffer, g_sender_number, 1);

		    g_critical_alertes = GSM_CRITICAL_ALERTE_ON;

		    g_send_to_masterIO[0] = 1;
		    send_msg_to_masterIO(GSM_IO_COMMAND_CRITICAL_TIME, g_send_to_masterIO, 1);
		}
		else if (strcmp(g_sms_recv_buffer, "Allume la lumiere 1") == 0)
		{
		    g_send_to_masterIO[0] = 1;
		    send_msg_to_masterIO(GSM_IO_COMMAND_LIGHT_1, g_send_to_masterIO, 1);
		}
		else if (strcmp(g_sms_recv_buffer, "Eteint la lumiere 1") == 0)
		{
		    g_send_to_masterIO[0] = 0;
		    send_msg_to_masterIO(GSM_IO_COMMAND_LIGHT_1, g_send_to_masterIO, 1);
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


