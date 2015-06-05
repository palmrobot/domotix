#include <WaveHC.h>
#include <WaveUtil.h>


/********************************************************/
/*      Pin  definitions                                */
/********************************************************/



/********************************************************/
/*      Serial GSM  definitions                         */
/********************************************************/

#define CMD_RECV_DATA_MAX			6
uint8_t g_recv_from_mother[CMD_RECV_DATA_MAX];

#define COMMAND_RECV_START			0xFE /* [0xFE Start transmission] */

#define COMMAND_RECV_TEST			0x80 /* [0x80 Test] [Nb of bytes] [byte 1] [byte 2] ... [byte n] */
#define COMMAND_RECV_TEST2			0x81 /* [0x81 Test2] [data1] [data2] */
#define COMMAND_RECV_INIT			0x82 /* [0x82 Init] */
#define COMMAND_RECV_SMS1			0x83 /* [0x83 SMS1] */
#define COMMAND_RECV_SMS2			0x84 /* [0x84 SMS2] */


#define CMD_SEND_DATA_MAX			16
uint8_t g_send_to_mother[CMD_SEND_DATA_MAX];

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


uint8_t g_process_check_gsm;

/********************************************************/
/*      Global definitions                              */
/********************************************************/



void setup()
{
    /* Initialize the output pins for the DAC control. */


    /* init process states */
    g_process_receive   = PROCESS_RECEIVE_WAIT_COMMAND;
    g_process_command   = 0;
    g_process_action    = PROCESS_ACTION_INIT;

    /* Init global variables */


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


void process_check_gsm(void)
{
    if (g_process_check_gsm)
    {


    }
}

void process_action(void)
{
    if (g_process_action)
    {
	if (g_process_action == PROCESS_ACTION_INIT)
	{
	    /* init GSM */


	    /* Then send OK is ready */
	    g_send_to_mother[0] = COMMAND_SEND_INIT_OK;
	    send_mother(g_send_to_mother, 1);
	}
	else if (g_process_action == PROCESS_ACTION_SMS1)
	{

	}
	else if (g_process_action == PROCESS_ACTION_SMS2)
	{

	}
    }
}

void loop()
{
    process_recv_command();
    process_send_command();
    process_action();
}


