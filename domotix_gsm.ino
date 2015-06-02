#include <WaveHC.h>
#include <WaveUtil.h>


/********************************************************/
/*      Pin  definitions                                */
/********************************************************/



/********************************************************/
/*      Serial GSM  definitions                         */
/********************************************************/
#define COMMAND_START			0xFE /* [0xFE Start transmission] */

#define COMMAND_TEST			0x80 /* [0x80 Test] [Nb of bytes] [byte 1] [byte 2] ... [byte n] */
#define COMMAND_TEST2			0x81 /* [0x81 Test2] [data1] [data2] */
#define COMMAND_READY			0x82 /* [0x82 Ready] */
#define COMMAND_INIT_ERROR		0x83 /* [0x84 End of file] */
#define COMMAND_INIT_FAT_ERROR		0x84 /* [0x84 End of file] */
#define COMMAND_INIT_ROOT_ERROR		0x85 /* [0x85 End of file] */
#define COMMAND_PLAYING_FILE		0x86 /* [0x86 File is playing] */
#define COMMAND_PLAY_END		0x87 /* [0x87 End of file] */
#define COMMAND_FILE_NUMBER		0x88 /* [0x88 Number of file] */

#define CMD_SEND_DATA_MAX		16
uint8_t g_send_mother[CMD_SEND_DATA_MAX];

#define CMD_DATA_MAX			6
uint8_t g_recv_mother[CMD_DATA_MAX];

/********************************************************/
/*      Process definitions                             */
/********************************************************/

#define PROCESS_RECEIVE_DO_NOTHING		0
#define PROCESS_RECEIVE_WAIT_COMMAND		1
uint8_t g_process_recv_command;

#define PROCESS_ACTION_INIT			0x01
uint16_t g_process_action;

#define PROCESS_SEND_LIST			0xD1 /* [0xD1 List] */
#define PROCESS_SEND_FILENAME			0xD2 /* [0xD2 Number of the file to get name ] */
#define PROCESS_SEND_PLAYFILE			0xD3 /* [0xD3 Play this file number */
#define PROCESS_SEND_STOP_PLAYING		0xD4 /* [0xD4 Stop playing] */
#define PROCESS_SEND_BEEP_KEY			0xD5 /* [0xD5 Playing Beep] */
#define PROCESS_SEND_NOTE			0xD6 /* [0xD6 Playing Note] */
#define PROCESS_SEND_MOTOR			0xD7 /* [0xD7 Playing motor sound] */
#define PROCESS_SEND_HELLO			0xD8 /* [0xD8 Playing hello sound] */

#define PROCESS_SEND_START			0xFE /* [0xFE Start transmission */
uint8_t g_process_send_command;

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
    Serial.write(COMMAND_START);

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

    if (g_process_recv_command)
    {
	/* if we get a valid char, read char */
	if (Serial.available() > 0)
	{
	    /* get Start Byte */
	    value = Serial.read();
	    if (value == COMMAND_START)
	    {
		/* Wait for serial */
		while (Serial.available() < CMD_DATA_MAX);

		for(g_recv_mother_nb = 0; g_recv_mother_nb < CMD_DATA_MAX; g_recv_mother_nb++)
		{
		    /* get incoming write: */
		    g_recv_mother[g_recv_mother_nb] = Serial.read();
		}

		/* Set action plan */
		g_process_action = g_recv_mother[0];

		/* Disable communication ,wait for message treatment */
		g_process_recv_command  = 0;
	    }
	}
    }
}


void process_send_command(void)
{

    if (g_process_send_command)
    {
	if (g_process_send_command == PROCESS_SEND_LIST)
	{
	    g_send_mother[0] = 0;
	    g_send_mother[1] = 1;
	    send_mother(g_send_mother, 2);
	}

	g_process_recv_command = PROCESS_RECEIVE_WAIT_COMMAND;
	g_process_send_command = 0;
    }
}

void process_action(void)
{
    if (g_process_action)
    {
	if ((g_process_action & PROCESS_ACTION_INIT) == PROCESS_ACTION_INIT)
	{
	    /* init GSM */


	    /* Then send OK is ready */
	    g_send_mother[0] = COMMAND_READY;
	    send_mother(g_send_mother, 1);

	    g_process_action &= ~PROCESS_ACTION_INIT;
	}
    }
}

void loop()
{
    process_recv_command();
    process_send_command();
    process_action();
}


