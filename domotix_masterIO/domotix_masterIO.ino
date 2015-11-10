#include <avr/pgmspace.h>
#include <SPI.h>
#include <Time.h>
#include <SD.h>
#include <Ethernet.h>

/* #define DEBUG */
/* #define DEBUG_HTML */
/* #define DEBUG_SENSOR */
/* #define DEBUG_ITEM */
/* #define DEBUG_SMS*/

#define VERSION				"v3.10"

/********************************************************/
/*      Pin  definitions                                */
/********************************************************/
#define PIN_A0				A0
#define PIN_GARAGE_LUMIERE_ETABLI	A1 /* D */
#define PIN_CELLIER_LUMIERE		A2 /* G */
#define PIN_TEMP_EXT			A3 /* M */
#define PIN_GARAGE_LUMIERE		A4 /* I */
#define PIN_LINGERIE_LUMIERE		A5 /* J */

#define PIN_GARAGE_DROITE		9 /* A */
#define PIN_GARAGE_GAUCHE		8 /* B */
#define PIN_GARAGE_FENETRE		7 /* C */
#define PIN_CELLIER_PORTE_EXT		6 /* E */
#define PIN_CELLIER_PORTE_INT		5 /* F */
#define PIN_LINGERIE_CUISINE		3 /* K */
#define PIN_GARAGE_FOND			2 /* H */
#define PIN_CUISINE_EXT			12 /* L */
#define PIN_LINGERIE_FENETRE		11 /* N */
#define PIN_ENTREE_PORTE_EXT		13 /* O */
#define PIN_POULAILLER_PORTE		14 /* P */
#define PIN_OUT_POULAILLER_ACTION	15
#define PIN_POULE_GAUCHE		16 /* R */
#define PIN_POULE_DROITE		17 /* S */

#define PIN_				10

#define PIN_OUT_GSM_INIT		36
#define PIN_OUT_LIGHT_1			40


#define PIN_SS_ETH_CONTROLLER 		50


/* Reserverd Pins */
#define CS_PIN_SDCARD			4
#define PIN_ETHER_CTRL1			50
#define PIN_ETHER_CTRL2			51
#define PIN_ETHER_CTRL3			52
#define PIN_ETHER_SELECT		53


#define LIGHT_OFF			1
#define LIGHT_ON			0


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

uint8_t g_recv_gsm[CMD_DATA_MAX];
uint8_t g_send_to_gsm[CMD_DATA_MAX];
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
/*      Process definitions                             */
/********************************************************/
#ifdef DEBUG
uint8_t g_process_serial;
#define PROCESS_SERIAL_OFF			0
#define PROCESS_SERIAL_ON			1
#endif

uint8_t g_process_ethernet;
#define PROCESS_ETHERNET_OFF			0
#define PROCESS_ETHERNET_ON			1

uint8_t g_process_domotix;
#define PROCESS_DOMOTIX_OFF			0
#define PROCESS_DOMOTIX_ON			1

uint8_t g_process_time;
#define PROCESS_TIME_OFF			0
#define PROCESS_TIME_ON				1

uint8_t g_process_schedule;
#define PROCESS_SCHEDULE_OFF			0
#define PROCESS_SCHEDULE_ON			1

uint8_t g_process_action;
#define PROCESS_ACTION_NONE			0
#define PROCESS_ACTION_GSM_INIT_OK		GSM_IO_COMMAND_INIT_OK
#define PROCESS_ACTION_GSM_INIT_FAILED		GSM_IO_COMMAND_INIT_FAILED
#define PROCESS_ACTION_LIGHT_1			GSM_IO_COMMAND_LIGHT_1
#define PROCESS_ACTION_CRITICAL_TIME		GSM_IO_COMMAND_CRITICAL_TIME

uint8_t g_process_recv_gsm;
#define PROCESS_RECV_GSM_DO_NOTHING		0
#define PROCESS_RECV_GSM_WAIT_COMMAND		1

/********************************************************/
/*      Global definitions                              */
/********************************************************/


typedef struct state_porte_s
{
    uint8_t old;
    uint8_t curr;
};

typedef struct state_lumiere_s
{
    int old;
    int curr;
    int state_old;
    int state_curr;
};

state_porte_s g_garage_droite; /* A */
state_porte_s g_garage_gauche; /* B */
state_porte_s g_garage_fenetre; /* C */
state_lumiere_s g_garage_lumiere_etabli; /* D */
state_porte_s g_cellier_porte_ext; /* E */
state_porte_s g_cellier_porte; /* F */
state_lumiere_s g_cellier_lumiere; /* G */
state_porte_s g_garage_porte; /* H */
state_lumiere_s g_garage_lumiere; /* I */
state_lumiere_s g_lingerie_lumiere; /* J */
state_porte_s g_lingerie_porte_cuisine; /* K */
state_porte_s g_cuisine_porte_ext; /* L */
state_lumiere_s g_temperature_ext; /* M */
state_porte_s g_lingerie_fenetre; /* N */
state_porte_s g_entree_porte_ext; /* O */
state_porte_s g_poulailler_porte; /* P */
state_porte_s g_poule_gauche; /* R */
state_porte_s g_poule_droite; /* S */

#define THRESHOLD_CMP_OLD	10
#define THRESHOLD_LIGHT_ON	220

uint16_t g_req_count = 0;
uint8_t g_debug      = 0;
uint8_t g_critical_time = 0;

uint8_t g_sched_temperature = 0;
uint8_t g_sched_door_already_opened = 0;
uint8_t g_sched_door_already_closed = 0;


/********************************************************/
/*      GSM global definitions                          */
/********************************************************/

/********************************************************/
/*      Ethernet global definitions                     */
/********************************************************/
uint8_t g_mac_addr[] = { 0x90, 0xA2, 0xDA, 0x00, 0xFF, 0x86};
uint8_t g_ip_addr[] = { 192, 168, 5, 20 };

#define LINE_MAX_LEN			64
char g_line[LINE_MAX_LEN + 1];

#define BUFF_HTML_MAX_SIZE		50
char g_buff_html[BUFF_HTML_MAX_SIZE];

#define BUFF_MAX_SIZE			250
char g_buff[BUFF_MAX_SIZE];

char g_full_list_name[13];

#define SIZE_CODE_HTML			10
char g_code_html[SIZE_CODE_HTML];

#define MAX_FILE_SIZE			4000 /* 4Ko */

#define FILE_MAX_LEN			13
typedef struct file_web_t
{
    File  fd;
    char name[FILE_MAX_LEN + 1];
};

typedef struct
{
    const char *name[2];
}code_t;

#define WEB_CODE_PORTE		'1'
#define WEB_CODE_LUMIERE	'2'
#define WEB_CODE_CLASS		'3'
#define WEB_CODE_VOLET		'4'
#define WEB_CODE_CLASS_POULE	'5'
#define WEB_CODE_POULE		'6'


#define TYPE_PORTE		0
#define TYPE_LUMIERE		1
#define TYPE_CLASS		2
#define TYPE_VOLET		3
#define TYPE_CLASS_POULE	4
#define TYPE_POULE		5

code_t g_code[] = { {"Ouverte", "Fermee"},  /* TYPE_PORTE*/
		    {"Allumee", "Eteinte"}, /* TYPE_LUMIERE */
		    {"ko", "ok"}, /* TYPE_CLASS */
		    {"Ouvert", "Ferme"}, /* TYPE_VOLET */
		    {"po", "vi"}, /* TYPE_CLASS_POULE */
		    {"Poule presente", "Vide"}}; /* TYPE_POULE */

/*
 * 12/12/14
 * 09:35:42
 * F
 * ok
 */
typedef struct
{
    char item;
    char date[12];
    char hour[12];
    char state[4];
    char clas[4];
}data_item_t;

#define NB_ITEM				7
data_item_t g_data_item[NB_ITEM];

#define SEPARATE_ITEM			'#'

#define STATE_SEPARATION		1
#define STATE_DATE			2
#define STATE_HOUR			3
#define STATE_STATE			4
#define STATE_CLASS			5


uint16_t g_time_to_open[13]  = {0, 825 , 732 , 636 , 623 , 540 , 538 , 613 , 700 , 701 , 735 , 739 , 827};
uint16_t g_time_to_close[13] = {0, 1652, 1738, 1829, 2020, 2108, 2151, 2202, 2131, 2031, 1924, 1722, 1645};

EthernetServer g_server(9090);
EthernetClient g_client;

#define WEB_GET			1
#define WEB_EOL			2
#define WEB_END			4
#define WEB_ERROR		16

uint8_t g_page_web   = 0;
time_t g_prevDisplay = 0;
file_web_t g_filename;
char  g_clock[9];
char  g_date[9];
uint8_t g_init = 0;
uint8_t g_init_gsm = 0;

/********************************************************/
/*      NTP			                        */
/********************************************************/

#define TIMEZONE			2
#define LOCAL_PORT_NTP			8888
#define NTP_PACKET_SIZE			48
#define TIME_SYNCHRO_SEC		200

/* server Free */
IPAddress g_timeServer(132, 163, 4, 101);
EthernetUDP g_Udp;
uint32_t g_beginWait;
time_t prevDisplay = 0;

/*  NTP time is in the first 48 bytes of message*/
byte g_packetBuffer[NTP_PACKET_SIZE];
uint8_t g_NTP = 0;


/********************************************************/
/*      CONSTANT STRING		                        */
/********************************************************/



/********************************************************/
/*      Init			                        */
/********************************************************/
void setup(void)
{
    uint8_t i;

    /* Init Input Ports */
    pinMode(PIN_GARAGE_DROITE, INPUT);
    pinMode(PIN_GARAGE_GAUCHE, INPUT);
    pinMode(PIN_GARAGE_FENETRE, INPUT);
    pinMode(PIN_CELLIER_PORTE_EXT, INPUT);
    pinMode(PIN_CELLIER_PORTE_INT, INPUT);
    pinMode(PIN_LINGERIE_CUISINE, INPUT);
    pinMode(PIN_GARAGE_FOND, INPUT);
    pinMode(PIN_CUISINE_EXT, INPUT);
    pinMode(PIN_LINGERIE_FENETRE, INPUT);
    pinMode(PIN_ENTREE_PORTE_EXT, INPUT);
    pinMode(PIN_POULAILLER_PORTE, INPUT);
    pinMode(PIN_POULE_GAUCHE, INPUT);
    pinMode(PIN_POULE_DROITE, INPUT);

    /* Init Output Ports */
    pinMode(PIN_OUT_LIGHT_1, OUTPUT);
    pinMode(PIN_OUT_GSM_INIT, OUTPUT);
    pinMode(PIN_OUT_POULAILLER_ACTION, OUTPUT);

    digitalWrite(PIN_OUT_LIGHT_1, LIGHT_OFF);
    digitalWrite(PIN_OUT_GSM_INIT, 0);
    digitalWrite(PIN_OUT_POULAILLER_ACTION, 0);

    /* init Process */
#ifdef DEBUG
    g_process_serial   = PROCESS_SERIAL_ON;
#endif

    g_process_ethernet = PROCESS_ETHERNET_ON;
    g_process_domotix  = PROCESS_DOMOTIX_ON;
    g_process_time     = PROCESS_TIME_OFF;
    g_process_schedule = PROCESS_SCHEDULE_ON;
    g_process_action   = PROCESS_ACTION_NONE;
    g_process_recv_gsm = PROCESS_RECV_GSM_WAIT_COMMAND;

#ifdef DEBUG
    /* initialize serial communications at 115200 bps */
    Serial.begin(9600);
    delay(100);
#endif

    /* initialize the serial communications with GSM Board */
    Serial.begin(9600);
    delay(100);

    /* start the Ethernet connection and the server: */
    Ethernet.begin(g_mac_addr, g_ip_addr);
    g_server.begin();
    delay(100);

    /* Init SDCard */
    SD.begin(CS_PIN_SDCARD);
    delay(100);

    /* init UDP for NTP */
    g_Udp.begin(LOCAL_PORT_NTP);
    setSyncInterval(TIME_SYNCHRO_SEC);
    setSyncProvider(getNtpTime);
    delay(100);

    /* Init global var */
    g_debug = 0;
    g_NTP   = 0;
    g_init  = 1;
    g_init_gsm = 0;
    g_recv_state = CMD_STATE_START;
    g_recv_size = 0;
    g_recv_index = 0;

    /* init pipes */
    g_recv_gsm[0]	= 0;

    /* Init global var for web code */
    g_garage_droite.old = 0;
    g_garage_gauche.old = 0;
    g_garage_fenetre.old = 0;
    g_garage_lumiere_etabli.old = 0;
    g_garage_lumiere_etabli.state_old = 0;
    g_garage_lumiere_etabli.state_curr = 0;
    g_garage_lumiere.old = 0;
    g_garage_lumiere.state_old = 0;
    g_garage_lumiere.state_curr = 0;
    g_garage_porte.old = 0;

    g_cellier_porte_ext.old = 0;
    g_cellier_porte.old = 0;
    g_cellier_lumiere.old = 0;
    g_cellier_lumiere.state_old = 0;
    g_cellier_lumiere.state_curr = 0;

    g_lingerie_lumiere.old = 0;
    g_lingerie_lumiere.state_old = 0;
    g_lingerie_lumiere.state_curr = 0;
    g_lingerie_porte_cuisine.old = 0;
    g_lingerie_fenetre.old = 0;
    g_entree_porte_ext.old = 0;

    g_cuisine_porte_ext.old = 0;

    g_poulailler_porte.old = 0;
    g_poule_gauche.old = 0;
    g_poule_droite.old = 0;

    g_temperature_ext.old = 0;

    g_sched_temperature = 0;
    g_sched_door_already_opened = 0;
    g_sched_door_already_closed = 0;

    for(i = 0; i < NB_ITEM; i++ )
    {
	g_data_item[i].item = 0;
	g_data_item[i].date[0] = 0;
	g_data_item[i].hour[0] = 0;
	g_data_item[i].state[0] = 0;
	g_data_item[i].clas[0] = 0;
    }

    send_gsm(IO_GSM_COMMAND_IS_INIT, NULL, 0);

#ifdef DEBUG
    PgmPrint("Free RAM: ");
    Serial.println(FreeRam());

    PgmPrintln("Init OK");
#endif
}

/** Store and print a string in flash memory.*/
#define PgmClientPrint(x) ClientPrint_P(PSTR(x))

/** Store and print a string in flash memory followed by a CR/LF.*/
#define PgmClientPrintln(x) ClientPrintln_P(PSTR(x))

/*
 * %Print a string in flash memory to the serial port.
 *
 * \param[in] str Pointer to string stored in flash memory.
 */
void ClientPrint_P(PGM_P str)
{
    uint8_t c;

    c = (uint8_t)pgm_read_byte(str);
    while (c != 0)
    {
	g_client.write(c);
	str++;
	c = (uint8_t)pgm_read_byte(str);
    }
}

/*
 * %Print a string in flash memory followed by a CR/LF.
 *
 * \param[in] str Pointer to string stored in flash memory.
 */
void ClientPrintln_P(PGM_P str)
{
  ClientPrint_P(str);
  g_client.println();
}



void send_gsm(uint8_t cmd, uint8_t *buffer, uint8_t size)
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

/** Store a string in flash memory.*/
#define send_SMS(x) send_SMS_P(PSTR(x))

void send_SMS_P(PGM_P str)
{
    uint8_t i;

    /* if (g_init_gsm == 0) */
    /*  	return; */

    i = 0;
    g_send_to_gsm[0] = pgm_read_byte(str);

    while ((i < (CMD_DATA_MAX-1)) && (g_send_to_gsm[i] != 0))
    {
	i++;
	str++;
	g_send_to_gsm[i] = pgm_read_byte(str);
    }

    if (i > 0)
    {
	send_gsm(IO_GSM_COMMAND_SMS, g_send_to_gsm, i);
    }
}

/********************************************************/
/*      Web                                             */
/********************************************************/
void deal_with_file(char item, char type, char code)
{
    data_item_t  *ptr_item;

    g_full_list_name[0] = '\0';
    sprintf(g_full_list_name, "%c.TXT", item);

    /* open file */
    read_item_in_file(item, g_full_list_name);

    switch (type)
    {
	case '1':
	{
	    ptr_item = &g_data_item[0];
	}break;
	case '2':
	{
	    ptr_item = &g_data_item[1];
	}break;
	case '3':
	{
	    ptr_item = &g_data_item[2];
	}break;
	case '4':
	{
	    ptr_item = &g_data_item[3];
	}break;
	case '5':
	{
	    ptr_item = &g_data_item[4];
	}break;
	case '6':
	{
	    ptr_item = &g_data_item[5];
	}break;
	case '7':
	{
	    ptr_item = &g_data_item[6];
	}break;

	default:

	break;
    }

   switch (code)
    {
	case '0':
	{
	    g_client.write((uint8_t*)ptr_item->date,strlen(ptr_item->date));
	}break;
	case '1':
	{
	    g_client.write((uint8_t*)ptr_item->hour,strlen(ptr_item->hour));
	}break;
	case '2':
	{
	    g_client.write((uint8_t*)ptr_item->state,strlen(ptr_item->state));
	}break;
	case '3':
	{
	    g_client.write((uint8_t*)ptr_item->clas,strlen(ptr_item->clas));
	}break;
	case '4':
	{
	    g_client.write((uint8_t*)ptr_item->poule,strlen(ptr_item->poule));
	}break;
	default:
	break;
    }
}

void deal_with_code(char item, char type, char code)
{
    code_t *ptr_code;

    switch (code)
    {
	case WEB_CODE_PORTE:
	{
	    ptr_code = &g_code[TYPE_PORTE];
	}break;
	case WEB_CODE_LUMIERE:
	{
	    ptr_code = &g_code[TYPE_LUMIERE];
	}break;
	case WEB_CODE_CLASS:
	{
	    ptr_code = &g_code[TYPE_CLASS];
	}break;
	case WEB_CODE_VOLET:
	{
	    ptr_code = &g_code[TYPE_VOLET];
	}break;
	case WEB_CODE_CLASS_POULE:
	{
	    ptr_code = &g_code[TYPE_CLASS_POULE];
	}break;
	case WEB_CODE_POULE:
	{
	    ptr_code = &g_code[TYPE_POULE];
	}break;
	default:
	{
	    ptr_code = &g_code[0];
	}break;
    }

    switch (item)
    {
	case 'w':
	{
	    if (g_critical_time)
	    {
		PgmClientPrint("Systeme d'alertes active");
	    }
	}break;
	case 'x':
	{
	    g_client.print(VERSION);
	}
	break;
	case 'y':
	{
	    g_debug = 1;
	}break;
	case 'z':
	{
	    /* save current date and clock in global var */
	    digitalClock();
	    digitalDate();
	    if (g_NTP)
	    {
		g_client.print(g_clock);
		g_client.print("  ");
		g_client.print(g_date);
	    }

	    /* debug code ( $y00 ) must be set after time in .html page */
	    g_debug = 0;
	}break;
	case 'A':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_garage_droite.curr],
		strlen(ptr_code->name[g_garage_droite.curr]));
	}break;
	case 'B':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_garage_gauche.curr],
		strlen(ptr_code->name[g_garage_gauche.curr]));
	}break;
	case 'C':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_garage_fenetre.curr],
		strlen(ptr_code->name[g_garage_fenetre.curr]));
	}break;
	case 'D':
	{
	    if (g_debug)
	    {
		g_client.print(g_garage_lumiere_etabli.curr);
	    }
	    else
	    {
		g_client.write((uint8_t*)ptr_code->name[g_garage_lumiere_etabli.state_curr],
		    strlen(ptr_code->name[g_garage_lumiere_etabli.state_curr]));
	    }
	}break;
	case 'E':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_cellier_porte_ext.curr],
		strlen(ptr_code->name[g_cellier_porte_ext.curr]));
	}break;
	case 'F':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_cellier_porte.curr],
		strlen(ptr_code->name[g_cellier_porte.curr]));
	}break;
	case 'G':
	{
	    if (g_debug)
	    {
		g_client.print(g_cellier_lumiere.curr);
	    }
	    else
	    {
		g_client.write((uint8_t*)ptr_code->name[g_cellier_lumiere.state_curr],
		    strlen(ptr_code->name[g_cellier_lumiere.state_curr]));
	    }
	}break;
	case 'H':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_garage_porte.curr],
		strlen(ptr_code->name[g_garage_porte.curr]));
	}break;
	case 'I':
	{
	    if (g_debug)
	    {
		g_client.print(g_garage_lumiere.curr);
	    }
	    else
	    {
		g_client.write((uint8_t*)ptr_code->name[g_garage_lumiere.state_curr],
		    strlen(ptr_code->name[g_garage_lumiere.state_curr]));
	    }
	}break;
	case 'J':
	{
	    if (g_debug)
	    {
		g_client.print(g_lingerie_lumiere.curr);
	    }
	    else
	    {
		g_client.write((uint8_t*)ptr_code->name[g_lingerie_lumiere.state_curr],
		    strlen(ptr_code->name[g_lingerie_lumiere.state_curr]));
	    }
	}break;
	case 'K':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_lingerie_porte_cuisine.curr],
		strlen(ptr_code->name[g_lingerie_porte_cuisine.curr]));
	}break;
	case 'L':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_cuisine_porte_ext.curr],
		strlen(ptr_code->name[g_cuisine_porte_ext.curr]));
	}break;
	case 'M':
	{
	    g_client.print(g_temperature_ext.curr);
	}break;
	case 'N':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_lingerie_fenetre.curr],
		strlen(ptr_code->name[g_lingerie_fenetre.curr]));
	}break;
	case 'O':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_entree_porte_ext.curr],
		strlen(ptr_code->name[g_entree_porte_ext.curr]));
	}break;
	case 'P':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_poulailler_porte.curr],
		strlen(ptr_code->name[g_poulailler_porte.curr]));
	}break;
	case 'R':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_poule_gauche.curr],
		strlen(ptr_code->name[g_poule_gauche.curr]));
	}break;
	case 'S':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_poule_droite.curr],
		strlen(ptr_code->name[g_poule_droite.curr]));
	}break;
	default:

	break;
    }
}


void deal_with_full_list(char item, char type, char code)
{
    sprintf(g_full_list_name, "%c.TXT", item);

    if (item == 'M')
	send_file_full_list(g_full_list_name, 4);
    else
	send_file_full_list(g_full_list_name, 5);

}

void send_file_to_client(File *file)
{
    uint16_t index;
    char code;
    char type;
    char item;

    index = 0;
    while (file->available())
    {
	g_buff_html[index] = file->read();

	if (g_buff_html[index] == '$')
	{
	    /* first of all, send the last buffer without the '$'*/
	    g_client.write((uint8_t*)g_buff_html, index);
	    index = 0;

	    /* then get the item */
	    item = file->read();

	    /* then get the type */
	    type = file->read();

	    /* then get the code */
	    code = file->read();

	    if (type == '0')
	    {
		deal_with_code(item, type, code);
	    }
	    else if (type == 'A')
	    {
		deal_with_full_list(item, type, code);
	    }
	    else
	    {
		deal_with_file(item, type, code);
	    }
	}
	else
	{
	    index ++;
	    if (index >= BUFF_HTML_MAX_SIZE)
	    {
		g_client.write((uint8_t*)g_buff_html, index);
		index = 0;
	    }
	}
    }
    if (index > 0)
	g_client.write((uint8_t*)g_buff_html, index);
}


void send_resp_to_client(File *fd)
{
    uint16_t index;

    index = 0;
    while (fd->available())
    {
	g_buff_html[index] = fd->read();
	index++;
	if (index >= BUFF_HTML_MAX_SIZE)
	{
	    g_client.write((uint8_t*)g_buff_html, index);
	    index = 0;
	}
    }
    if (index > 0)
	g_client.write((uint8_t*)g_buff_html, index);
}

void send_file_full_list(char *file, uint8_t nb_item)
{
    File  fd;
    uint8_t step;
    uint8_t value;

    /* write in file
     * Format :
     * #
     * 12/12/2014
     * 09:35:42
     * F
     * ok
     */

    fd = SD.open(file, FILE_READ);

    step  = 0;
    while (fd.available())
    {
	value = fd.read();

	if (value == '\n')
	{
	    step++;
	    if (step < nb_item )
	    {
		g_client.print(" -- ");
	    }
	    else
	    {
		step = 0;
		g_client.print("</br>");
	    }
	}
	else
	    g_client.write(value);
    }

    fd.close();
}

/********************************************************/
/*      NTP functions                                   */
/********************************************************/

#ifdef DEBUG
void digitalClockDisplaySerial(void)
{
    /* save current date and clock in global var */
    digitalClock();
    digitalDate();
    Serial.print(g_clock);
    Serial.print("  ");
    Serial.print(g_date);
    Serial.println();
}
#endif

void digitalClock(void)
{
    sprintf(g_clock,"%02d:%02d:%02d",hour(), minute(), second());
}

void digitalDate(void)
{
    sprintf(g_date,"%02d/%02d/%02d",day(), month(), year()-2000);
}

time_t getNtpTime(void)
{
    int size;
    unsigned long secsSince1900;

    /* discard any previously received packets */
    while (g_Udp.parsePacket() > 0);

#ifdef DEBUG
    Serial.println("Transmit NTP");
#endif

    sendNTPpacket(g_timeServer);

    g_beginWait = millis();
    while (millis() - g_beginWait < 1500)
    {
	size = g_Udp.parsePacket();

	if (size >= NTP_PACKET_SIZE)
	{

#ifdef DEBUG
	    Serial.println("Receive NTP");
#endif

	    g_Udp.read(g_packetBuffer, NTP_PACKET_SIZE);

	    /* NTP is ok and running */
	    g_NTP = 1;

	    /* convert four bytes starting at location 40 to a long integer */
	    secsSince1900 =  (unsigned long)g_packetBuffer[40] << 24;
	    secsSince1900 |= (unsigned long)g_packetBuffer[41] << 16;
	    secsSince1900 |= (unsigned long)g_packetBuffer[42] << 8;
	    secsSince1900 |= (unsigned long)g_packetBuffer[43];
	    return secsSince1900 - 2208988800UL + TIMEZONE * SECS_PER_HOUR;
	}
    }
#ifdef DEBUG
    Serial.println("No NTP :-(");
#endif
    return 0;
}

/* send an NTP request to the time server at the given address */
void sendNTPpacket(IPAddress &address)
{
    /* set all bytes in the buffer to 0 */
    memset(g_packetBuffer, 0, NTP_PACKET_SIZE);

    /* Initialize values needed to form NTP request */
    /* (see URL above for details on the packets) */
    g_packetBuffer[0] = 0b11100011;   /* LI, Version, Mode */
    g_packetBuffer[1] = 0;     /* Stratum, or type of clock */
    g_packetBuffer[2] = 6;     /* Polling Interval */
    g_packetBuffer[3] = 0xEC;  /* Peer Clock Precision */

    /* 8 bytes of zero for Root Delay & Root Dispersion */
    g_packetBuffer[12]  = 49;
    g_packetBuffer[13]  = 0x4E;
    g_packetBuffer[14]  = 49;
    g_packetBuffer[15]  = 52;

    /* all NTP fields have been given values, now
     * you can send a packet requesting a timestamp:
     * NTP requests are to port 123
     */
    g_Udp.beginPacket(address, 123);
    g_Udp.write(g_packetBuffer, NTP_PACKET_SIZE);
    g_Udp.endPacket();
}

/********************************************************/
/*      Process                                         */
/********************************************************/

#ifdef DEBUG
void process_serial(void)
{
    uint16_t count;
    uint8_t command;

    if (g_process_serial != PROCESS_SERIAL_OFF)
    {
	/* if we get a valid byte, read analog ins: */
	count = Serial.available();
	while (count > 0)
	{
	    /* get incoming byte: */
	    command = Serial.read();
	    switch (command)
	    {
		case 'a':
		{
			    digitalClockDisplaySerial();
		}break;
	    }

	    count--;
	}
    }
}
#endif


void process_recv_gsm(void)
{
    uint8_t value;
    uint8_t i;
    uint8_t crc;
    uint8_t recv_crc;

    if (g_process_recv_gsm == PROCESS_RECV_GSM_WAIT_COMMAND)
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
		    g_recv_size  = Serial.read();

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
		while ((Serial.available() > 0) && (g_recv_index < g_recv_size))
		{
		    /* get incoming data: */
		    g_recv_gsm[g_recv_index] = Serial.read();
		    g_recv_crc += g_recv_gsm[g_recv_index];
		    g_recv_index++;

		    if (g_recv_index == g_recv_size)
		    {
			/* Set null terminated string */
			g_recv_gsm[g_recv_index] = 0;

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
			g_process_recv_gsm = PROCESS_RECV_GSM_WAIT_COMMAND;

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
		g_process_recv_gsm = PROCESS_RECV_GSM_DO_NOTHING;

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

void process_ethernet(void)
{
    uint8_t count;
    char *end_filename;
    char eat_req;
    int exit;

    if (g_process_ethernet != PROCESS_ETHERNET_OFF)
    {
	g_client = g_server.available();
	if (g_client)
	{
#ifdef DEBUG_HTML
	    PgmPrintln("New Connection");
#endif
	    g_page_web  = 0;
	    g_req_count = 0;
	    exit = 0;

	    while (g_client.connected() && (exit == 0))
	    {
		if (g_client.available())
		{
		    g_line[g_req_count] = g_client.read();
#ifdef DEBUG_HTML
		    Serial.print(g_line[g_req_count]);
#endif

		    /* Search for end of line */
		    if ((g_line[g_req_count] == '\r') ||
			(g_line[g_req_count] == '\n'))
		    {
#ifdef DEBUG_HTML
		        PgmPrintln("");
#endif
			/* Got a complete line */
			/* Set '\0' to the end of buffer for string treatment */
			g_line[g_req_count] = '\0';

			/* EOL */
			g_page_web |= WEB_EOL;

			/* Buffer line is used, then go to the end
			 * to eat the end of the request
			 */
			g_req_count = LINE_MAX_LEN;

		    }

		    g_req_count++;

		    if (g_req_count >= LINE_MAX_LEN)
		    {
			/* Line is too long, eat all the request and return error */
			while (g_client.available())
			{
			    eat_req = g_client.read();
			}

			exit = 1;
		    }
		}
		else
		{
		    exit = 1;
		}
	    }
	    /********** Parsing Line received **************/
	    if ((g_page_web & WEB_EOL) == WEB_EOL)
	    {
		if (strstr(g_line, "GET /") != NULL)
		{
		    g_filename.name[0] = '\0';

		    if (g_line[5] == ' ')
		    {
			/* No file specified, then use the default one */
			strcpy(g_filename.name,"index.htm");

			g_filename.fd = SD.open(g_filename.name, FILE_READ);
		    }
		    else
		    {
			end_filename = strstr(g_line," HTTP");
			if (end_filename != NULL)
			{
			    *end_filename = '\0';

			    if (strlen(g_line+5) < FILE_MAX_LEN)
			    {
				strcpy(g_filename.name, &g_line[5]);

#ifdef DEBUG_HTML
				PgmPrint("found file: ");Serial.println(g_filename.name);
#endif
				g_filename.fd = SD.open(g_filename.name, FILE_READ);
			    }
			}
		    }

		    if (g_filename.fd.available())
		    {
			/* send 200 OK */
			PgmClientPrintln("HTTP/1.1 200 OK");

			if (strstr(g_filename.name, ".htm") != NULL)
			{
			    PgmClientPrintln("Content-Type: text/html");

			    /* end of header */
			    g_client.println();
			    send_file_to_client(&g_filename.fd);
			    g_filename.fd.close();
			}
			else
			{
			    if (strstr(g_filename.name, ".css") != NULL)
			    {
				PgmClientPrintln("Content-Type: text/css");
			    }
			    else if (strstr(g_filename.name, ".jpg") != NULL)
			    {
				PgmClientPrintln("Content-Type: image/jpeg");
				PgmClientPrintln("Cache-Control: max-age=2592000");
			    }
			    else if (strstr(g_filename.name, ".ico") != NULL)
			    {
				PgmClientPrintln("Content-Type: image/x-icon");
				PgmClientPrintln("Cache-Control: max-age=2592000");
			    }
			    else
				PgmClientPrintln("Content-Type: text");

			    /* end of header */
			    g_client.println();

			    send_resp_to_client(&g_filename.fd);
			    g_filename.fd.close();
			}
		    }
		    else
		    {
			PgmClientPrintln("HTTP/1.1 404 Not Found");
			PgmClientPrintln("Content-Type: text/html");
			g_client.println();
			PgmClientPrintln("<h2>Domotix Error: File Not Found!</h2>");
		    }
		}
		else
		{
		    PgmClientPrintln("HTTP/1.1 404 Not Found");
		    PgmClientPrintln("Content-Type: text/html");
		    g_client.println();
		    PgmClientPrintln("<h2>Domotix Error: GET /  Not Found!</h2>");
		}
	    }
	    else
	    {
		PgmClientPrintln("HTTP/1.1 404 Not Found");
		PgmClientPrintln("Content-Type: text/html");
		g_client.println();
		PgmClientPrintln("<h2>Domotix Error: GET /  Error, line too long!</h2>");
	    }
	    /* close connection */
	    delay(5);
	    g_client.stop();

#ifdef DEBUG_HTML
	    PgmPrintln("Connection Closed");
#endif
	}
    }
}

void read_item_in_file(char item_value, char *file)
{
    uint32_t	file_size;
    int32_t	index;
    File	fd;
    uint8_t	nb_item;
    uint8_t	item;
    uint8_t	state;
    uint8_t	i,j;
    char	value;

    /* read file
     * Format :
     * #
     * 12/12/2014
     * 09:35:42
     * F
     * ok
     */

#ifdef DEBUG_ITEM
    PgmPrint("File: ");Serial.println(file);
#endif

    if (g_data_item[0].item == item_value)
    {
	return;
    }
    else
    {
	/* reset table */
	for(i = 0; i < NB_ITEM; i++ )
	{
	    g_data_item[i].item = 0;
	    g_data_item[i].date[0] = 0;
	    g_data_item[i].hour[0] = 0;
	    g_data_item[i].state[0] = 0;
	    g_data_item[i].clas[0] = 0;
	}
	/* set new table */
	g_data_item[0].item = item_value;
    }


#ifdef DEBUG_ITEM
    PgmPrintln("Continue");
#endif

    fd = SD.open(file, FILE_READ);
    if (fd == 0)
    {

#ifdef DEBUG_ITEM
	PgmPrintln("File not found");
#endif
	return;
    }

    file_size = fd.size();

#ifdef DEBUG_ITEM
    PgmPrint("File size = ");Serial.print(file_size);PgmPrintln(" Bytes");
#endif

    if (file_size == 0)
    {
	fd.close();
	return;
    }

    if (file_size > BUFF_MAX_SIZE)
	fd.seek(file_size - BUFF_MAX_SIZE);
    else
	fd.seek(0);

    /* read the last part of the file */
    index = 0;
    while (fd.available() && (index < BUFF_MAX_SIZE))
    {
	g_buff[index] = fd.read();
	if (g_buff[index] != -1)
	    index++;
    }
    index--;

    fd.close();

#ifdef DEBUG_ITEM
    PgmPrintln("search for the last 7 item");
#endif

    /* search for the last 7 items */
    nb_item = 0;
    while((index > 0) && (nb_item < 7))
    {
	if (g_buff[index] == SEPARATE_ITEM)
	{
	    nb_item++;
	}
	index--;
    }

#ifdef DEBUG_ITEM
    PgmPrint("Found ");Serial.print(nb_item);PgmPrintln(" items");
#endif

    /* Save items to struct */
    state = STATE_SEPARATION;
    j     = 0;
    item  = 0;

    /* read file until the end */
    while (nb_item > 0)
    {
	index++;
	switch (state)
	{
	    case STATE_SEPARATION:
	    {
		if (g_buff[index] == '\n')
		{
		    /* switch to next state */
		    state = STATE_DATE;
		    j     = 0;
		}
	    }break;
	    case STATE_DATE:
	    {
		if (g_buff[index] == '\n')
		{
		    /* set end of string for last item */
		    g_data_item[item].date[j] = 0;

		    /* switch to next state */
		    state = STATE_HOUR;
		    j     = 0;
		}
		else
		{
		    g_data_item[item].date[j] = g_buff[index];
		    j++;
		}
	    }break;
	    case STATE_HOUR:
	    {
		if (g_buff[index] == '\n')
		{
		    /* set end of string for last item */
		    g_data_item[item].hour[j] = 0;

		    /* switch to next state */
		    state = STATE_STATE;
		    j     = 0;
		}
		else
		{
		    g_data_item[item].hour[j] = g_buff[index];
		    j++;
		}
	    }break;
	    case STATE_STATE:
	    {
		if (g_buff[index] == '\n')
		{
		    /* set end of string for last item */
		    g_data_item[item].state[j] = 0;

		    /* switch to next state */
		    state = STATE_CLASS;
		    j     = 0;
		}
		else
		{
		    g_data_item[item].state[j] = g_buff[index];
		    j++;
		}
	    }break;
	    case STATE_CLASS:
	    {
		if (g_buff[index] == '\n')
		{
		    /* set end of string for last item */
		    g_data_item[item].clas[j] = 0;

		    /* switch to next state */
		    state = STATE_SEPARATION;
		    j     = 0;
		    item++;
		    nb_item--;

		}
		else
		{
		    g_data_item[item].clas[j] = g_buff[index];
		    j++;
		}
	    }break;
	}
    }


#ifdef DEBUG_ITEM
    PgmPrint("Nb item = ");Serial.println(item);


    for(j=0;j<item ;j++ )
    {

	PgmPrint("nb item = ");Serial.print(j+1);Serial.print("/");Serial.println(item);
	PgmPrint("date    = ");Serial.println(g_data_item[j].date);
	PgmPrint("hour    = ");Serial.println(g_data_item[j].hour);
	PgmPrint("state   = ");Serial.println(g_data_item[j].state);
	PgmPrint("class   = ");Serial.println(g_data_item[j].clas);
    }
#endif

}

void copy_file(const char *file)
{
    File  fd;
    File  fd_backup;
    uint32_t sizefile;
    char value_read;
    char file_date[15];

    fd = SD.open(file, FILE_READ);

    /* Check file size */
    sizefile = fd.size();

    if (sizefile > MAX_FILE_SIZE)
    {
	/* create new file */
	sprintf(file_date,"%c%02d%02d%02d.TXT",file[0], day(), month(), year()-2000);

	fd_backup = SD.open(file_date, FILE_WRITE);

	/* copy the file to his backup */
	while (fd.available())
	{
	    value_read = fd.read();
	    fd_backup.write((const uint8_t*)value_read, 1);
	}
	fd_backup.close();
	fd.close();

	/* and then remove file to empty it*/
	SD.remove((char*)file);
    }
    else
    {
	fd.close();
    }
}

void save_entry(const char *file, uint8_t value, uint8_t type)
{
    File  fd;
    File  fd_backup;
    code_t *class_html;
    code_t *state;
    uint32_t sizefile;
    char value_read;
    char file_date[15];

    /* write in file
     * Format :
     * #
     * 12/12/2014
     * 09:35:42
     * F
     * ok
     */

    /* Date must be ready to save entry */
    if (g_NTP == 0)
	return;

    /* check if file is too big, then save it */
    copy_file(file);

    fd = SD.open(file, FILE_WRITE);

    fd.println(SEPARATE_ITEM);
    fd.println(g_date);
    fd.println(g_clock);
    state = (code_t*)&g_code[type];
    fd.write((const uint8_t*)state->name[value], 1);
    fd.println();
    if (type == TYPE_POULE)
    {
	class_html = (code_t*)&g_code[TYPE_CLASS_POULE];
    }
    else
    {
	class_html = (code_t*)&g_code[TYPE_CLASS];
    }
    fd.println(class_html->name[value]);
    fd.close();
}

void save_entry_temp(const char *file, int value)
{
    File  fd;
    char data[4+1];

    /* write in file
     * Format :
     * 12/12/2014
     * 09:35:42
     * temp
     */

    /* Date must be ready to save entry */
    if (g_NTP == 0)
	return;

    /* check if file is too big, then save it */
    copy_file(file);

    fd = SD.open(file, FILE_WRITE);

    fd.println(SEPARATE_ITEM);
    fd.println(g_date);
    fd.println(g_clock);
    sprintf(data,"%d C", value);
    fd.println(data);
    fd.close();
}

void process_domotix(void)
{
    uint8_t wait_a_moment;
    int value;

    /* save current date and clock in global var */
    digitalClock();
    digitalDate();

    wait_a_moment = 0;
    if (g_process_domotix != PROCESS_DOMOTIX_OFF)
    {
	g_garage_droite.curr =  digitalRead(PIN_GARAGE_DROITE);
	if ((g_garage_droite.curr != g_garage_droite.old) || (g_init))
	{
	    g_garage_droite.old = g_garage_droite.curr;

	    /* write in file  */
	    save_entry("A.txt", g_garage_droite.curr, TYPE_PORTE);

#ifdef DEBUG_SENSOR
	    PgmPrint("Garage droite :");Serial.println(g_garage_droite.curr);
#endif

	    /* Critical part */
	    if ((g_garage_droite.curr) && (g_init == 0))
	    {
		/* Send SMS */
		send_SMS("La porte de droite du garage vient de s'ouvrir");
	    }

	    wait_a_moment = 1;
	}

	g_garage_gauche.curr =  digitalRead(PIN_GARAGE_GAUCHE);
	if ((g_garage_gauche.curr != g_garage_gauche.old) || (g_init))
	{
	    g_garage_gauche.old = g_garage_gauche.curr;

	    /* write in file  */
	    save_entry("B.txt", g_garage_gauche.curr, TYPE_PORTE);

#ifdef DEBUG_SENSOR
	    PgmPrint("Garage gauche :");Serial.println(g_garage_gauche.curr);
#endif

	    /* Critical part */
	    if ((g_garage_gauche.curr) && (g_init == 0))
	    {
		/* Send SMS */
		send_SMS("La porte de gauche du garage vient de s'ouvrir");
	    }

	    wait_a_moment = 1;
	}

	g_garage_fenetre.curr =  digitalRead(PIN_GARAGE_FENETRE);
	if ((g_garage_fenetre.curr != g_garage_fenetre.old) || (g_init))
	{
	    g_garage_fenetre.old = g_garage_fenetre.curr;

	    /* write in file  */
	    save_entry("C.txt", g_garage_fenetre.curr, TYPE_PORTE);

#ifdef DEBUG_SENSOR
	    PgmPrint("Garage fenetre :");Serial.println(g_garage_fenetre.curr);
#endif

	    /* Critical part */
	    if ((g_garage_fenetre.curr) && (g_init == 0))
	    {
		/* Send SMS */
		send_SMS("La fenetre du garage vient de s'ouvrir");
	    }

	    wait_a_moment = 1;
	}

	g_cellier_porte_ext.curr =  digitalRead(PIN_CELLIER_PORTE_EXT);
	if ((g_cellier_porte_ext.curr != g_cellier_porte_ext.old) || (g_init))
	{
	    g_cellier_porte_ext.old = g_cellier_porte_ext.curr;

	    /* write in file  */
	    save_entry("E.txt", g_cellier_porte_ext.curr, TYPE_PORTE);

#ifdef DEBUG_SENSOR
	    PgmPrint("Cellier porte ext :");Serial.println(g_cellier_porte_ext.curr);
#endif

	    /* Critical part */
	    if ((g_cellier_porte_ext.curr) && (g_init == 0))
	    {
		/* Send SMS */
		send_SMS("La porte exterieure du cellier vient de s'ouvrir");
	    }

	    wait_a_moment = 1;
	}

	g_cellier_porte.curr =  digitalRead(PIN_CELLIER_PORTE_INT);
	if ((g_cellier_porte.curr != g_cellier_porte.old) || (g_init))
	{
	    g_cellier_porte.old = g_cellier_porte.curr;

	    /* write in file  */
	    save_entry("F.txt", g_cellier_porte.curr, TYPE_PORTE);

#ifdef DEBUG_SENSOR
	    PgmPrint("Cellier porte lingerie :");Serial.println(g_cellier_porte.curr);
#endif
	    wait_a_moment = 1;
	}

	g_lingerie_porte_cuisine.curr =  digitalRead(PIN_LINGERIE_CUISINE);
	if ((g_lingerie_porte_cuisine.curr != g_lingerie_porte_cuisine.old) || (g_init))
	{
	    g_lingerie_porte_cuisine.old = g_lingerie_porte_cuisine.curr;

	    /* write in file  */
	    save_entry("K.txt", g_lingerie_porte_cuisine.curr, TYPE_PORTE);

#ifdef DEBUG_SENSOR
	    PgmPrint("Lingerie porte cuisine :");Serial.println(g_lingerie_porte_cuisine.curr);
#endif
	    wait_a_moment = 1;
	}

	g_garage_porte.curr =  digitalRead(PIN_GARAGE_FOND);
	if ((g_garage_porte.curr != g_garage_porte.old) || (g_init))
	{
	    g_garage_porte.old = g_garage_porte.curr;

	    /* write in file  */
	    save_entry("H.txt", g_garage_porte.curr, TYPE_PORTE);

#ifdef DEBUG_SENSOR
	    PgmPrint("Garage fond :");Serial.println(g_garage_porte.curr);
#endif
	    wait_a_moment = 1;
	}

	g_cuisine_porte_ext.curr = digitalRead(PIN_CUISINE_EXT);
	if ((g_cuisine_porte_ext.curr != g_cuisine_porte_ext.old) || (g_init))
	{
	    g_cuisine_porte_ext.old = g_cuisine_porte_ext.curr;

	    /* write in file  */
	    save_entry("L.txt", g_cuisine_porte_ext.curr, TYPE_PORTE);

#ifdef DEBUG_SENSOR
	    PgmPrint("Cuisine porte ext:");Serial.println(g_cuisine_porte_ext.curr);
#endif

	    /* Critical part */
	    if ((g_cuisine_porte_ext.curr) && (g_init == 0))
	    {
		/* Send SMS */
		send_SMS("La porte exterieure de la cuisine vient de s'ouvrir");
	    }

	    wait_a_moment = 1;
	}

	g_lingerie_fenetre.curr = digitalRead(PIN_LINGERIE_FENETRE);
	if ((g_lingerie_fenetre.curr != g_lingerie_fenetre.old) || (g_init))
	{
	    g_lingerie_fenetre.old = g_lingerie_fenetre.curr;

	    /* write in file  */
	    save_entry("N.txt", g_lingerie_fenetre.curr, TYPE_PORTE);

#ifdef DEBUG_SENSOR
	    PgmPrint("Lingerie fenetre:");Serial.println(g_lingerie_fenetre.curr);
#endif
	    /* Critical part */
	    if ((g_lingerie_fenetre.curr) && (g_init == 0))
	    {
		/* Send SMS */
		send_SMS("La Fenetre de la lingerie vient de s'ouvrir");
	    }

	    wait_a_moment = 1;
	}

	g_entree_porte_ext.curr = digitalRead(PIN_ENTREE_PORTE_EXT);
	if ((g_entree_porte_ext.curr != g_entree_porte_ext.old) || (g_init))
	{
	    g_entree_porte_ext.old = g_entree_porte_ext.curr;

	    /* write in file  */
	    save_entry("O.txt", g_entree_porte_ext.curr, TYPE_PORTE);

#ifdef DEBUG_SENSOR
	    PgmPrint("Entree Porte Ext:");Serial.println(g_entree_porte_ext.curr);
#endif

	    /* Critical part */
	    if ((g_entree_porte_ext.curr) && (g_init == 0))
	    {
		/* Send SMS */
		send_SMS("La porte d'entree vient de s'ouvrir");
	    }

	    wait_a_moment = 1;
	}

	g_poulailler_porte.curr = digitalRead(PIN_POULAILLER_PORTE);
	if ((g_poulailler_porte.curr != g_poulailler_porte.old) || (g_init))
	{
	    g_poulailler_porte.old = g_poulailler_porte.curr;

	    /* write in file  */
	    save_entry("P.txt", g_poulailler_porte.curr, TYPE_PORTE);

#ifdef DEBUG_SENSOR
	    PgmPrint("Poulailler Porte :");Serial.println(g_poulailler_porte.curr);
#endif

	    wait_a_moment = 1;
	}

	g_poule_gauche.curr = digitalRead(PIN_POULE_GAUCHE);
	if ((g_poule_gauche.curr != g_poule_gauche.old) || (g_init))
	{
	    g_poule_gauche.old = g_poule_gauche.curr;

	    /* write in file  */
	    save_entry("R.txt", g_poule_gauche.curr, TYPE_POULE);

#ifdef DEBUG_SENSOR
	    PgmPrint("Poule gauche :");Serial.println(g_poule_gauche.curr);
#endif

	    wait_a_moment = 1;
	}

	g_poule_droite.curr = digitalRead(PIN_POULE_DROITE);
	if ((g_poule_droite.curr != g_poule_droite.old) || (g_init))
	{
	    g_poule_droite.old = g_poule_droite.curr;

	    /* write in file  */
	    save_entry("S.txt", g_poule_droite.curr, TYPE_POULE);

#ifdef DEBUG_SENSOR
	    PgmPrint("Poule droite :");Serial.println(g_poule_droite.curr);
#endif

	    wait_a_moment = 1;
	}




	/* ================================
	 *
	 *
	 * Analog
	 *
	 *
	 *
	 * =================================
	 */

	g_garage_lumiere_etabli.curr = analogRead(PIN_GARAGE_LUMIERE_ETABLI);
	if ( (g_init) || (g_garage_lumiere_etabli.curr > (g_garage_lumiere_etabli.old + THRESHOLD_CMP_OLD)) ||
	    ((g_garage_lumiere_etabli.curr + THRESHOLD_CMP_OLD) < g_garage_lumiere_etabli.old))
	{
	    g_garage_lumiere_etabli.old = g_garage_lumiere_etabli.curr;

	    if (g_garage_lumiere_etabli.curr > THRESHOLD_LIGHT_ON)
	    {
		g_garage_lumiere_etabli.state_curr = 0;
	    }
	    else
	    {
		g_garage_lumiere_etabli.state_curr = 1;
	    }

	    if ((g_garage_lumiere_etabli.state_curr != g_garage_lumiere_etabli.state_old)  || (g_init))
	    {
		g_garage_lumiere_etabli.state_old = g_garage_lumiere_etabli.state_curr;

		/* write in file  */
		save_entry("D.txt", g_garage_lumiere_etabli.state_curr, TYPE_LUMIERE);
	    }

#ifdef DEBUG_SENSOR
	    PgmPrint("Garage lumiere etabli :");Serial.println(g_garage_lumiere_etabli.curr);
#endif
	    wait_a_moment = 1;
	}

	g_cellier_lumiere.curr =  analogRead(PIN_CELLIER_LUMIERE);
	if ( (g_init) || (g_cellier_lumiere.curr > (g_cellier_lumiere.old + THRESHOLD_CMP_OLD)) ||
	    ((g_cellier_lumiere.curr + THRESHOLD_CMP_OLD) < g_cellier_lumiere.old))
	{
	    g_cellier_lumiere.old = g_cellier_lumiere.curr;

	    if (g_cellier_lumiere.curr > THRESHOLD_LIGHT_ON)
	    {
		g_cellier_lumiere.state_curr = 0;
	    }
	    else
	    {
		g_cellier_lumiere.state_curr = 1;
	    }

	    if ((g_cellier_lumiere.state_curr != g_cellier_lumiere.state_old) || (g_init))
	    {
		g_cellier_lumiere.state_old = g_cellier_lumiere.state_curr;

		/* write in file  */
		save_entry("G.txt", g_cellier_lumiere.state_curr, TYPE_LUMIERE);
	    }

#ifdef DEBUG_SENSOR
	    PgmPrint("Cellier lumiere :");Serial.println(g_cellier_lumiere.curr);
#endif
	    wait_a_moment = 1;
	}

	g_garage_lumiere.curr =  analogRead(PIN_GARAGE_LUMIERE);
	if ( (g_init) || (g_garage_lumiere.curr > (g_garage_lumiere.old + THRESHOLD_CMP_OLD)) ||
	    ((g_garage_lumiere.curr + THRESHOLD_CMP_OLD) < g_garage_lumiere.old))
	{
	    g_garage_lumiere.old = g_garage_lumiere.curr;

	    if (g_garage_lumiere.curr > THRESHOLD_LIGHT_ON)
	    {
		g_garage_lumiere.state_curr = 0;
	    }
	    else
	    {
		g_garage_lumiere.state_curr = 1;
	    }

	    if ((g_garage_lumiere.state_curr != g_garage_lumiere.state_old) || (g_init))
	    {
		g_garage_lumiere.state_old = g_garage_lumiere.state_curr;

		/* write in file  */
		save_entry("I.txt", g_garage_lumiere.state_curr, TYPE_LUMIERE);
	    }

#ifdef DEBUG_SENSOR
	    PgmPrint("Garage lumiere :");Serial.println(g_garage_lumiere.curr);
#endif
	    wait_a_moment = 1;
	}

	g_lingerie_lumiere.curr =  analogRead(PIN_LINGERIE_LUMIERE);
	if ( (g_init) || (g_lingerie_lumiere.curr > (g_lingerie_lumiere.old + THRESHOLD_CMP_OLD)) ||
	    ((g_lingerie_lumiere.curr + THRESHOLD_CMP_OLD) < g_lingerie_lumiere.old))
	{
	    g_lingerie_lumiere.old = g_lingerie_lumiere.curr;

	    if (g_lingerie_lumiere.curr > THRESHOLD_LIGHT_ON)
	    {
		g_lingerie_lumiere.state_curr = 0;
	    }
	    else
	    {
		g_lingerie_lumiere.state_curr = 1;
	    }

	    if ((g_lingerie_lumiere.state_curr != g_lingerie_lumiere.state_old) || (g_init))
	    {
		g_lingerie_lumiere.state_old = g_lingerie_lumiere.state_curr;

		/* write in file  */
		save_entry("J.txt", g_lingerie_lumiere.state_curr, TYPE_LUMIERE);
	    }

#ifdef DEBUG_SENSOR
	    PgmPrint("Lingerie lumiere :");Serial.println(g_lingerie_lumiere.curr);
#endif
	    wait_a_moment = 1;
	}

	/* ================================
	 *
	 *
	 * Temperature
	 *
	 *
	 *
	 * =================================
	 */

	value = analogRead(PIN_TEMP_EXT);
	g_temperature_ext.curr = (500.0 * value) / 1024;

	if (wait_a_moment)
	{
	    /* wait some time, before testing the next time the inputs */
	    delay(500);
	}

	/* reset state of init */
	g_init = 0;
    }
}


void process_time(void)
{
    time_t  curr_time;

    if (g_process_time != PROCESS_TIME_OFF)
    {
	/* if (timeStatus() != timeNotSet) */
	/* { */
	/*     curr_time = now(); */
	/*     if (curr_time != g_prevDisplay) */
	/*     { */
	/* 	g_prevDisplay = curr_time; */
	/* 	digitalClockDisplay(); */
	/*     } */
	/* } */
    }
}


void process_schedule(void)
{
    uint16_t time;
    uint16_t date;
    uint8_t  mon;

    if (g_process_schedule != PROCESS_SCHEDULE_OFF)
    {
	/* get time values */
	time = (100*hour()) + minute();
	date = (100*month()) + day();
	mon = month();

	/*************************************/
	/* Scheduling for temperature sensor */
	if ((time == 800) || (time == 1400) || (time == 2000))
	{
	    if (g_sched_temperature == 0)
	    {
		g_sched_temperature = 1;

		/* save current date and clock in global var */
		digitalClock();
		digitalDate();

		/* write in file  */
		save_entry_temp("M.txt", g_temperature_ext.curr);
	    }
	}
	else
	{
	    g_sched_temperature = 0;
	}

	if ((g_init_gsm == 0) && ((time & 0x2) == time))
	{
	    send_gsm(IO_GSM_COMMAND_IS_INIT, NULL, 0);
	}

	if (time == g_time_to_open[mon])
	{
	    if (g_sched_door_already_opened == 0)
	    {
		g_sched_door_already_opened = 1;

		/* Open the door */
		digitalWrite(PIN_OUT_POULAILLER_ACTION, 1);
		delay(500);
		digitalWrite(PIN_OUT_POULAILLER_ACTION, 0);
	    }
	}
	else
	{
	    g_sched_door_already_opened = 0;
	}

	if (time == g_time_to_close[mon])
	{
	    if (g_sched_door_already_closed == 0)
	    {
		g_sched_door_already_closed = 1;

		/* close the door */
		digitalWrite(PIN_OUT_POULAILLER_ACTION, 1);
		delay(500);
		digitalWrite(PIN_OUT_POULAILLER_ACTION, 0);
	    }
	}
	else
	{
	    g_sched_door_already_closed = 0;
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
	    case PROCESS_ACTION_LIGHT_1:
	    {
		if (g_recv_gsm[0] == 1)
		{
		    digitalWrite(PIN_OUT_LIGHT_1, LIGHT_ON);
		}
		else
		{
		    digitalWrite(PIN_OUT_LIGHT_1, LIGHT_OFF);
		}
	    }break;

	    case PROCESS_ACTION_CRITICAL_TIME:
	    {
		if (g_recv_gsm[0] == 0)
		{
		    g_critical_time = 0;
		}
		else
		{
		    g_critical_time = 1;
		}
	    }
	    break;

	    /*
	     * Cases not oftem used
	     */

	    case PROCESS_ACTION_GSM_INIT_FAILED:
	    {

	    }break;

	    case PROCESS_ACTION_GSM_INIT_OK:
	    {
		digitalWrite(PIN_OUT_GSM_INIT, 1);
		g_init_gsm = 1;
	    }break;

	    default:
	    {
	    }
	    break;
	}

	if (action_done)
	{
	    /* start waiting for an other comand */
	    g_process_recv_gsm = PROCESS_RECV_GSM_WAIT_COMMAND;

	    /* Reset action, wait for next one */
	    g_process_action = PROCESS_ACTION_NONE;
	}

    }
}


void loop(void)
{
    process_ethernet();
#ifdef DEBUG
    process_serial();
#endif
    process_recv_gsm();
    process_time();
    process_domotix();
    process_schedule();
    process_action();
}
