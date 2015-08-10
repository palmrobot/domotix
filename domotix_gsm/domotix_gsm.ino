#include <WaveHC.h>
#include <WaveUtil.h>
#include <GSM.h>

/********************************************************/
/*      State  definitions                              */
/********************************************************/
/*
Master Board                   GSM Board
    |                             |
  Start                         Start
    |                             |
   Init                        Init GSM
    |                             |
  Ask for ready ? INIT----------->
    |                             |
  Recv Ready <--------- INIT_OK Ready
    |                             |
  Ready to send message           |
*/

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
#define PIN_CODE				"2153"
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
/*      Serial GSM  definitions                         */
/********************************************************/

#define CMD_DATA_MAX			6
uint8_t g_recv_from_mother[CMD_DATA_MAX];

#define COMMAND_RECV_START			0xFE /* [0xFE Start transmission] */

#define COMMAND_RECV_TEST			0x80 /* [0x80 Test] [Nb of bytes] [byte 1] [byte 2] ... [byte n] */
#define COMMAND_RECV_TEST2			0x81 /* [0x81 Test2] [data1] [data2] */
#define COMMAND_RECV_INIT			0x82 /* [0x82 Init] */
#define COMMAND_RECV_SMS1			0x83 /* [0x83 SMS1] */
#define COMMAND_RECV_SMS2			0x84 /* [0x84 SMS2] */


uint8_t g_send_to_mother[CMD_DATA_MAX];

#define COMMAND_SEND_START			0xFE /* [0xFE Start transmission */
#define COMMAND_SEND_INIT_OK			0xD1 /* [0xD1 Init OK] */
#define COMMAND_SEND_INIT_FAILED		0xD2 /* [0xD2 Init Failed ] */
#define COMMAND_SEND_ACTION_SMS1		0xD3 /* [0xD3 SMS For Action 1 ] */
#define COMMAND_SEND_ACTION_SMS2		0xD4 /* [0xD4 SMS For Action 2 ] */


/********************************************************/
/*      Process definitions                             */
/********************************************************/


#define PROCESS_RECEIVE_WAIT_COMMAND		1
uint8_t g_process_recv_command;

#define PROCESS_ACTION_INIT			COMMAND_RECV_INIT
#define PROCESS_ACTION_SMS1			COMMAND_RECV_SMS1
#define PROCESS_ACTION_SMS2			COMMAND_RECV_SMS2
uint8_t g_process_action;


uint8_t g_process_recv_gsm_sms;

/********************************************************/
/*      Global definitions                              */
/********************************************************/

#define GSM_INIT_FAILED				0
#define GSM_INIT_OK				1
uint8_t g_gsm_init;


void setup()
{
    /* Initialize the output pins for the DAC control. */


    /* init process states */
    g_process_receive		= PROCESS_RECEIVE_WAIT_COMMAND;
    g_process_recv_command	= 0;
    g_process_action		= PROCESS_ACTION_INIT;
    g_process_recv_gsm_sms	= 0;

    /* Init global variables */
    g_gsm_init			 = GSM_INIT_FAILED;

    /* init pipes */
    g_recv_mother[0]	= 0;
    g_recv_mother_nb	= 0;

    g_send_mother[0]	= 0;

    /* initialize serial communications at 115200 bps: */
    Serial.begin(115200);

    delay(100);
}

/*  GSM   -> Ethernet */
/*  Start ->       */
/*  Cmd   ->       */
/*  Data1 ->       */
/*  Data2 ->       */
/*  Data3 ->       */
/*  Data4 ->       */
void send_mother(uint8_t *buffer, int len)
{
    uint8_t padding[CMD_SEND_DATA_MAX] = {0};

    if (len > CMD_SEND_DATA_MAX)
	len = CMD_SEND_DATA_MAX;

    /* Send Start of transmission */
    Serial.write(COMMAND_SEND_START);

    /* Write Command + Data */
    Serial.write(buffer, len);

    /* Write padding Data */
    Serial.write(padding, CMD_SEND_DATA_MAX - len);
}


/********************************************************/
/*      Process Functions                               */
/********************************************************/
void process_recv_command(void)
{
    uint8_t value;

    if (g_process_recv_command == PROCESS_RECEIVE_WAIT_COMMAND)
    {
	/* if we get a valid char, read char */
	if (Serial.available() > 0)
	{
	    /* get Start Byte */
	    value = Serial.read();
	    if (value == COMMAND_RECV_START)
	    {
		/* Wait for serial */
		while (Serial.available() < CMD_RECV_DATA_MAX);

		for(g_recv_mother_nb = 0; g_recv_mother_nb < CMD_RECV_DATA_MAX; g_recv_mother_nb++)
		{
		    /* get incoming write: */
		    g_recv_from_mother[g_recv_from_mother_nb] = Serial.read();
		}

		/* Set action plan */
		g_process_action = g_recv_from_mother[0];

		/* Disable communication ,wait for message treatment */
		g_process_recv_command  = 0;
	    }
	}
    }
}


void process_recv_gsm_sms(void)
{
    uint8_t i;

    if (g_process_recv_gsm_sms)
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
		g_sms_buffer[i] = sms.read();
		i++;
	    }
	    while (g_sms_buffer[i-1] != 0);

	    /* Delete message from modem memory */
	    g_gsm_sms.flush();

	    /* Check sender */
	    if (strstr(g_sender_number,"+33668089948") != NULL)
	    {
		
		

	    }
	    else if (strstr(g_sender_number,"+33668088058") != NULL)
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
		if(g_gsm_access.begin(PIN_CODE) == GSM_READY)
		{
		    not_connected = false;
		}
		else
		{
		    delay(500);
		}
	    }

	    /* Then send OK is ready */
	    g_send_to_mother[0] = COMMAND_SEND_INIT_OK;
	    send_mother(g_send_to_mother, 1);

	    g_gsm_init = GSM_INIT_OK;

	    /* start polling sms */
	    g_process_recv_gsm_sms = 1;

	    /* restart waiting for command */
	    g_process_recv_command == PROCESS_RECEIVE_WAIT_COMMAND;

	}
	else if (g_process_action == PROCESS_ACTION_SMS1)
	{

	}
	else if (g_process_action == PROCESS_ACTION_SMS2)
	{

	}

	/* end of action, wait for a new one */
	g_process_action = 0;
    }
}

void loop()
{
    process_recv_command();
    process_action();
    process_recv_gsm_sms();
}


