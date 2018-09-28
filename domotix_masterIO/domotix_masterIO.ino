/*#include <DateTime.h>
  #include <DateTimeStrings.h>*/
#include <avr/pgmspace.h>
#include <SPI.h>
#include <Time.h>
#include <SD.h>
#include <Ethernet2.h>
#include <EEPROM.h>

/* #define DEBUG_SERIAL */
/* #define DEBUG_TEMP */
/* #define DEBUG_EVT */
/* #define DEBUG_EDF */
/* #define DEBUG_HTML */
/* #define DEBUG_SENSOR */
/* #define DEBUG_ITEM */
/* #define DEBUG_SMS*/
/* #define DEBUG_MEM */
/* #define DEBUG_NTP*/
/* #define DEBUG_METEO */

#define VERSION				"v5.27"

/********************************************************/
/*      Pin  definitions                               */
/********************************************************/

#define PIN_GARAGE_DROITE		9  /* A */
#define PIN_GARAGE_GAUCHE		8  /* B */
#define PIN_GARAGE_FENETRE		7  /* C */
#define PIN_GARAGE_LUMIERE_ETABLI	A1 /* D */
#define PIN_CELLIER_PORTE_EXT		6  /* E */
#define PIN_CELLIER_PORTE_INT		5  /* F */
#define PIN_CELLIER_LUMIERE		A2 /* G */
#define PIN_GARAGE_FOND			2  /* H */
#define PIN_GARAGE_LUMIERE		A4 /* I */
#define PIN_LINGERIE_LUMIERE		A5 /* J */
#define PIN_LINGERIE_CUISINE		3  /* K */
#define PIN_CUISINE_EXT			12 /* L */
#define PIN_TEMP_EXT			A3 /* M */
#define PIN_LINGERIE_FENETRE		11 /* N */
#define PIN_ENTREE_PORTE_EXT		13 /* O */
#define PIN_OUT_POULAILLER_ACTION	14 /* blue */
#define PIN_POULAILLER_PORTE		15 /* P yellow */
#define PIN_BUREAU_PORTE		28 /* Q */
#define PIN_POULE_GAUCHE		16 /* R grey */
#define PIN_POULE_DROITE		17 /* S brown */
#define PIN_POULAILLER_HALL		27 /* T */
#define PIN_TEMP_EXT_OFFSET		A0 /* U */
#define PIN_TEMP_GARAGE		        A6 /* V */
/* Week */				   /* W */
#define PIN_BUREAU_LUMIERE	        A7 /* X */
#define PIN_TEMP_BUREAU			A9 /* Y */
#define PIN_BUREAU_FENETRE		29 /* Z */
/* ADDR IP */				   /* a */
/* lampe1 */				   /* b */
/* lampe2 */				   /* c */
/* lampe3 */				   /* d */
/* lampe4 */				   /* e */
/* EDF HC */				   /* f */
/* EDF HP */				   /* g */
#define PIN_OUT_BUZZER			44 /* h */
/* temp day min */			   /* i */
/* temp day max */			   /* j */
/* temp year min */			   /* k */
/* temp year max */			   /* l */
/* EDF HC week */			   /* m */
/* EDF HP week */			   /* n */
#define PIN_TEMP_GRENIER		A10/* o */
#define PIN_GRENIER_LUMIERE	        A11/* p */
#define PIN_METEO_GIROUETTE	        A12/* q */
#define PIN_METEO_ANEMOMETRE	        18 /* r */
#define PIN_METEO_PLUVIOMETRE	        19 /* s */
/* Pluviometrie nuit */			   /* t */
/* Max Pluviometrie  */			   /* u */
/* Max speed day */			   /* , */
/* Max speed year */			   /* - */
/* Start Time */			   /* v */
/* Critical alertes */			   /* w */
/* Domotix version */			   /* x */
/* Debug part */			   /* y */
/* Date & hour */			   /* z */

#define PIN_GSM				26
#define PIN_EDF         		A8

#define PIN_OUT_EDF			37
#define PIN_OUT_GSM_INIT		36
#define PIN_OUT_LAMPE_1			39
#define PIN_OUT_LAMPE_2			38
#define PIN_OUT_LAMPE_3			41
#define PIN_OUT_LAMPE_4			40

#define PIN_SS_ETH_CONTROLLER 		50

/* Reserverd Pins */
#define CS_PIN_SDCARD			4
#define PIN_ETHER_CTRL1			50
#define PIN_ETHER_CTRL2			51
#define PIN_ETHER_CTRL3			52
#define PIN_ETHER_SELECT		53
#define PIN_UNUSED			10

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

uint8_t g_recv_gsm[CMD_DATA_MAX];
uint8_t g_send_to_gsm[CMD_DATA_MAX];

#define MAX_WAIT_SERIAL				500

/********************************************************/
/*      EEPROM Addresses                                */
/********************************************************/
#define EEPROM_ADDR_EDF_HC			0
#define EEPROM_ADDR_EDF_HP			4
#define EEPROM_ADDR_TIMEZONE			8
#define EEPROM_ADDR_MINYEAR			9
#define EEPROM_ADDR_MINYEAR_HOU			10
#define EEPROM_ADDR_MINYEAR_MIN			11
#define EEPROM_ADDR_MINYEAR_DAY			12
#define EEPROM_ADDR_MINYEAR_MON			13
#define EEPROM_ADDR_MAXYEAR			14
#define EEPROM_ADDR_MAXYEAR_HOU			15
#define EEPROM_ADDR_MAXYEAR_MIN			16
#define EEPROM_ADDR_MAXYEAR_DAY			17
#define EEPROM_ADDR_MAXYEAR_MON			18
#define EEPROM_ADDR_EDF_WEEK_HC			20
#define EEPROM_ADDR_EDF_WEEK_HP			24
#define EEPROM_ADDR_WEEK			28
#define EEPROM_ADDR_PLUVIO_MAXYEAR		29
#define EEPROM_ADDR_PLUVIO_MAXYEAR_DAY		31
#define EEPROM_ADDR_PLUVIO_MAXYEAR_MON		32
#define EEPROM_ADDR_ANEMO_MAXYEAR		33
#define EEPROM_ADDR_ANEMO_MAXYEAR_HOU		35
#define EEPROM_ADDR_ANEMO_MAXYEAR_MIN		36
#define EEPROM_ADDR_ANEMO_MAXYEAR_DAY		37
#define EEPROM_ADDR_ANEMO_MAXYEAR_MON		38

/*#define EEPROM_ADDR_NEXT			39 */


/********************************************************/
/*      Process definitions                             */
/********************************************************/
#define PROCESS_OFF			0
#define PROCESS_ON			1

#ifdef DEBUG_SERIAL
uint8_t g_process_serial;
#endif /* DEBUG_SERIAL */

#ifdef DEBUG_METEO
uint32_t g_cpt_milli = 0;
#endif /* METEO */

uint8_t g_process_ethernet;
uint8_t g_process_domotix;
uint8_t g_process_domotix_quick;
uint8_t g_process_time;
uint8_t g_process_schedule;
uint8_t g_process_delay;

volatile uint8_t g_process_action;
#define PROCESS_ACTION_NONE			PROCESS_OFF
#define PROCESS_ACTION_LIGHT_1			GSM_IO_COMMAND_LIGHT_1
#define PROCESS_ACTION_CRITICAL_TIME		GSM_IO_COMMAND_CRITICAL_TIME
#define PROCESS_ACTION_LAMPE			0x61
#define PROCESS_ACTION_EDF			0x62
#define PROCESS_ACTION_TIMEZONE			0x63

uint8_t g_process_recv_gsm;
#define PROCESS_RECV_GSM_DO_NOTHING		PROCESS_OFF
#define PROCESS_RECV_GSM_WAIT_COMMAND		PROCESS_ON

#define PROCESS_DOMOTIX_TIMEOUT			2000
#define PROCESS_DOMOTIX_CALC_CPT		50 /* => 50*PROCESS_DOMOTIX_TIMEOUT = 10000 msec = 10sec */

/********************************************************/
/*      Global definitions                              */
/********************************************************/


typedef struct state_porte_s
{
    uint8_t old;
    uint8_t curr;
    int8_t  id;
};

typedef struct state_lumiere_s
{
    uint32_t value;
    uint16_t old;
    uint16_t curr;
    uint8_t state_old;
    uint8_t state_curr;
    int8_t  id;
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
state_porte_s g_lingerie_fenetre; /* N */
state_porte_s g_entree_porte_ext; /* O */
state_porte_s g_poulailler_porte; /* P */
state_porte_s g_bureau_porte; /* Q */
state_porte_s g_poule_gauche; /* R */
state_porte_s g_poule_droite; /* S */
state_porte_s g_poulailler; /* T */
state_lumiere_s g_edf_hc;
state_lumiere_s g_edf_hp;
uint16_t	g_temperature_garage; /* V */
state_lumiere_s g_bureau_lumiere; /* X */
uint16_t	g_temperature_bureau; /* Y */
state_porte_s	g_bureau_fenetre; /* Z */
uint16_t	g_temperature_grenier; /* o */
state_lumiere_s g_grenier_lumiere; /* p */

#define THRESHOLD_CMP_OLD		10
#define THRESHOLD_LIGHT_ON_GARAGE	130
#define THRESHOLD_LIGHT_ON_ETABLI	500
#define THRESHOLD_LIGHT_ON_CELLIER	250
#define THRESHOLD_LIGHT_ON_LINGERIE	300
#define THRESHOLD_LIGHT_ON_BUREAU	340
#define THRESHOLD_LIGHT_ON_GRENIER	400
#define OFFSET_TEMP_BUREAU		2
#define OFFSET_TEMP_EXT			2
#define OFFSET_TEMP_GRENIER		2

#define LIGHT_OFF			0
#define LIGHT_ON			1

#define LAMPE_OFF			1
#define LAMPE_ON			0

#define THRESHOLD_EDF			160

uint16_t g_req_count = 0;
uint8_t g_debug      = 0;
uint8_t g_start	     = 0;

uint8_t g_sched_temperature = 0;
uint8_t g_sched_door_already_opened = 0;
uint8_t g_sched_door_already_closed = 0;
uint8_t g_sched_midnight = 0;
uint8_t g_sched_night = 0;
uint8_t g_sched_temp_year = 0;
uint8_t g_sched_daymin_sms = 0;

uint8_t g_lampe1 = LAMPE_OFF;
uint8_t g_lampe2 = LAMPE_OFF;
uint8_t g_lampe3 = LAMPE_OFF;
uint8_t g_lampe4 = LAMPE_OFF;

int8_t g_temperature_ext = 60;
int8_t g_temperature_daymin = 60;
int8_t g_temperature_daymax = -60;
int8_t g_temperature_yearmin = 60;
int8_t g_temperature_yearmax = -60;
int8_t g_temperature_yearmin_hour;
int8_t g_temperature_yearmin_min;
int8_t g_temperature_yearmin_day;
int8_t g_temperature_yearmin_mon;
int8_t g_temperature_yearmax_hour;
int8_t g_temperature_yearmax_min;
int8_t g_temperature_yearmax_day;
int8_t g_temperature_yearmax_mon;

char  g_tempdaymin_string[20]; /* 27°C à 17h32 */
char  g_tempdaymax_string[20];
char  g_tempyearmin_string[30]; /* 27°C à 17h32 le 23/03 */
char  g_tempyearmax_string[30];
int16_t  g_temperature_grenier_cpt = 0;
int16_t  g_temperature_grenier_total = 0;
int16_t  g_temperature_ext_cpt = 0;
int16_t  g_temperature_ext_total = 0;
int16_t  g_temperature_bureau_cpt = 0;
int16_t  g_temperature_bureau_total = 0;
int16_t  g_temperature_garage_cpt = 0;
int16_t  g_temperature_garage_total = 0;
char  g_pluvio_string[10]; /* 123mm */
char  g_pluvio_night_string[10]; /* 123mm */
char  g_pluvio_max_string[30]; /* 123mm le 23/03 */
char  g_anemo_string[30]; /* 132 km/h */
char  g_anemo_max_day_string[30]; /* 132 km/h */
char  g_anemo_max_year_string[30]; /* 132 km/h à 17h32 le 23/03*/
char  g_girouette_string[15]; /* Nord Ouest */

#define PLUVIO_UNIT     0.2794
#define ANEMO_UNIT_SEC  2.4
#define ANEMO_10_SEC    24

#define GIROUETTE_MAX_8		870 /* 930 */
#define GIROUETTE_MAX_7		770 /* 836 */
#define GIROUETTE_MAX_6		620 /* 732 */
#define GIROUETTE_MAX_5		480 /* 555 */
#define GIROUETTE_MAX_4		300 /* 386 */
#define GIROUETTE_MAX_3		180 /* 233 */
#define GIROUETTE_MAX_2		100 /* 132 */
#define GIROUETTE_MAX_1		50  /* 75 */

volatile uint16_t g_pluvio_cpt = 0;
volatile uint16_t g_pluvio_night_cpt = 0;
uint16_t g_pluvio_max_cpt = 0;
uint8_t g_pluvio_max_cpt_day = 0;
uint8_t g_pluvio_max_cpt_mon = 0;

volatile uint16_t g_anemo_cpt = 0;
uint16_t g_anemo_max_day_cpt = 0;
uint16_t g_anemo_max_year_cpt = 0;
uint8_t g_anemo_max_year_cpt_hour = 0;
uint8_t g_anemo_max_year_cpt_min = 0;
uint8_t g_anemo_max_year_cpt_day = 0;
uint8_t g_anemo_max_year_cpt_mon = 0;

volatile uint32_t g_beginWait1sec = 0;
volatile uint32_t g_beginWait3msec = 0;
uint16_t g_girouette = 0;
uint16_t g_girouette_cpt = 0;
uint16_t g_girouette_total = 0;

/********************************************************/
/*      GSM global definitions                          */
/********************************************************/

#define GSM_CRITICAL_ALERTE_OFF				0
#define GSM_CRITICAL_ALERTE_ON				1
uint8_t g_critical_alertes;

/********************************************************/
/*      Ethernet global definitions                     */
/********************************************************/
uint8_t g_mac_addr[] = { 0x90, 0xA2, 0xDA, 0x11, 0x1C, 0x44};
uint8_t g_ip_addr[] = { 192, 168, 5, 20 };

EthernetServer g_server(9090);
EthernetClient g_client;
uint8_t g_remoteIP[] = {0, 0, 0, 0};
//g_client.getRemoteIP(g_remoteIP); // where rip is defined as byte rip[] = {0,0,0,0 };

#define LINE_MAX_LEN			100
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
#define WEB_CODE_CHECKED	'7'
#define WEB_CODE_CRITICAL	'8'
#define WEB_CODE_NOTCHECKED	'9'


#define TYPE_PORTE		0
#define TYPE_LUMIERE		1
#define TYPE_CLASS		2
#define TYPE_VOLET		3
#define TYPE_CLASS_POULE	4
#define TYPE_POULE		5
#define TYPE_CHECKED		6
#define TYPE_CRITICAL		7
#define TYPE_NOTCHECKED		8


code_t g_code[] = { {"Ouverte", "Fermee"},  /* TYPE_PORTE*/
		    {"Allumee", "Eteinte"}, /* TYPE_LUMIERE */
		    {"ko", "ok"}, /* TYPE_CLASS */
		    {"Ouvert", "Ferme"}, /* TYPE_VOLET */
		    {"vi", "po"}, /* TYPE_CLASS_POULE */
		    {"Vide", "Poule"}, /* TYPE_POULE */
		    {"checked", ""}, /* TYPE_CHECKED */
		    {"cr", "ok"}, /* TYPE_CRITIQUE */
		    {"", "checked"}}; /* TYPE_NOTCHECKED */

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
    char state[8];
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

/*                              0  , 0  , jan, jan,  fev, fev, mar, mar, avr, avr, mai, mai, jun, jun, jul, jul, aou, aou, sep, sep, oct, oct, nov, nov, dec, dec  */
uint16_t g_time_to_open[26]  = {0  , 0  , 720, 705,  650, 635, 620, 600, 645, 620, 600, 545, 530, 530, 545, 600, 630, 600, 645, 700, 720, 735, 650, 705, 720, 735};

/*                              0  , 0  , jan , jan , fev , fev , mar , mar , avr , avr , mai , mai , jun , jun , jul , jul , aou , aou , sep , sep , oct , oct , nov , nov , dec , dec  */
uint16_t g_time_to_close[26] = {0  , 0  , 1800, 1820, 1840, 1900, 1930, 1955, 2110, 2130, 2145, 2200, 2215, 2230, 2205, 2150, 2140, 2120, 1955, 1930, 1900, 1840, 1820, 1800, 1800, 1800};

#define WEB_GET			1
#define WEB_EOL			2
#define WEB_END			4
#define WEB_ERROR		16

uint8_t g_page_web   = 0;
time_t g_prevDisplay = 0;
file_web_t g_filename;
char  g_clock[9];
char  g_date[9];
char  g_start_date[20];
uint8_t g_init_gsm = 0;

/********************************************************/
/*      Delay & Timers			                */
/********************************************************/

typedef void (*callback_delay) (void);

typedef struct
{
    uint32_t delay_start;
    uint32_t delay_wait;
    callback_delay cb;
    uint8_t  delay_inuse;
    uint8_t  *process;
}delay_t;

#define NB_DELAY_MAX			10
delay_t g_delay[NB_DELAY_MAX];

typedef struct
{
    uint8_t  id;
    uint32_t timeout;
}event_t;

event_t g_evt_buzz_before5min_on;
event_t g_evt_buzz_before5min_off;
event_t g_evt_cellier_porte_ext_open;
event_t g_evt_process_domotix;

/********************************************************/
/*      NTP			                        */
/********************************************************/

#define TIMEZONE			2
#define LOCAL_PORT_NTP			8888
#define NTP_PACKET_SIZE			48
#define TIME_SYNCHRO_SEC		200

/* server Free */
char g_timeServer[] = "ntp.midway.ovh";
EthernetUDP g_Udp;
uint32_t g_beginWait;
time_t prevDisplay = 0;

/*  NTP time is in the first 48 bytes of message*/
byte g_packetBuffer[NTP_PACKET_SIZE];
uint8_t g_NTP = 0;
uint8_t g_timezone = 1;

/* date & time */
uint8_t g_week = 0;
uint8_t g_hour = 0;
uint8_t g_min  = 0;
uint16_t g_hour100 = 0;
uint8_t g_sec  = 0;
uint8_t g_day  = 0;
uint8_t g_mon  = 0;
uint16_t g_year = 0;
char *g_tab_week[8] = {"Dimanche","Lundi","Mardi","Mercredi","Jeudi","Vendredi","Samedi","Dimanche"};

/* EDF */
uint32_t g_edf_week_hc = 0;
uint32_t g_edf_week_hp = 0;

/********************************************************/
/*      FUNCTION		                        */
/********************************************************/
void wait_some_time( uint8_t *process, unsigned long time_to_wait, callback_delay call_after_delay);

/********************************************************/
/*      CONSTANT STRING		                        */
/********************************************************/


/********************************************************/
/*      Interrupt		                        */
/********************************************************/

void interrupt_pluvio()
{
    g_pluvio_cpt++;
    g_pluvio_night_cpt++;
}

void interrupt_anemo()
{
    g_anemo_cpt++;
}

/********************************************************/
/*      Init			                        */
/********************************************************/
void setup(void)
{
    uint8_t i;
    int16_t value;
    int16_t value_offset;
    int16_t temp;
    uint16_t pluvio;
#ifdef DEBUG_MEM
    char eeprom_str[20];
    uint8_t hour;
#endif

    /* Init Input Ports */
    pinMode(PIN_METEO_PLUVIOMETRE, INPUT);
    pinMode(PIN_METEO_ANEMOMETRE, INPUT);
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
    pinMode(PIN_POULAILLER_HALL, INPUT);
    pinMode(PIN_BUREAU_PORTE, INPUT);
    pinMode(PIN_BUREAU_FENETRE, INPUT);
    pinMode(PIN_GSM, INPUT);

    /* Init Output Ports */
    pinMode(PIN_OUT_EDF, OUTPUT);
    pinMode(PIN_OUT_LAMPE_1, OUTPUT);
    pinMode(PIN_OUT_LAMPE_2, OUTPUT);
    pinMode(PIN_OUT_LAMPE_3, OUTPUT);
    pinMode(PIN_OUT_LAMPE_4, OUTPUT);
    pinMode(PIN_OUT_GSM_INIT, OUTPUT);
    pinMode(PIN_OUT_POULAILLER_ACTION, OUTPUT);
    pinMode(PIN_OUT_BUZZER, OUTPUT);

    g_lampe1 = LAMPE_OFF;
    g_lampe2 = LAMPE_OFF;
    g_lampe3 = LAMPE_OFF;
    g_lampe4 = LAMPE_OFF;

    digitalWrite(PIN_OUT_EDF, LIGHT_OFF);
    digitalWrite(PIN_OUT_LAMPE_1, g_lampe1);
    digitalWrite(PIN_OUT_LAMPE_2, g_lampe2);
    digitalWrite(PIN_OUT_LAMPE_3, g_lampe3);
    digitalWrite(PIN_OUT_LAMPE_4, g_lampe4);
    digitalWrite(PIN_OUT_GSM_INIT, LIGHT_OFF);
    digitalWrite(PIN_OUT_POULAILLER_ACTION, 0);
    analogWrite(PIN_OUT_BUZZER, 0);

    /* init Process */
#ifdef DEBUG_SERIAL
    g_process_serial   = PROCESS_ON;
#endif

    g_process_ethernet = PROCESS_ON;
    g_process_domotix  = PROCESS_ON;
    g_process_domotix_quick  = PROCESS_ON;
    g_process_time     = PROCESS_ON;
    g_process_delay    = PROCESS_ON;
    g_process_schedule = PROCESS_OFF;
    g_process_action   = PROCESS_ACTION_NONE;
    g_process_recv_gsm = PROCESS_RECV_GSM_WAIT_COMMAND;

#ifdef DEBUG_SERIAL
    /* initialize serial communications at 115200 bps */
    Serial.begin(115200);
    delay(100);
#endif

    /* initialize the serial communications with GSM Board */
    Serial.begin(115200);
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
    g_start = 0;

    g_pluvio_cpt = 0;
    g_pluvio_night_cpt = 0;
    g_pluvio_max_cpt = 0;

    g_anemo_cpt = 0;
    g_anemo_max_day_cpt = 0;
    g_anemo_max_year_cpt = 0;

    g_beginWait1sec = millis();
    g_beginWait3msec = g_beginWait1sec;

    g_girouette = 0;
    g_girouette_cpt = 0;
    g_girouette_total = 0;

    g_date[0] = 0;
    g_clock[0] = 0;
    g_start_date[0] = 0;

    g_init_gsm = 2;
    g_critical_alertes = 0;

    EEPROM.get(EEPROM_ADDR_TIMEZONE, g_timezone);
    if ((g_timezone != 1) && (g_timezone != 2))
    {
	g_timezone = 2;
	EEPROM.put(EEPROM_ADDR_TIMEZONE, g_timezone);
    }

    /* init pipes */
    g_recv_gsm[0]	= 0;

    /* Init global var for web code */
    g_garage_droite.old = 2;
    g_garage_droite.id = -1;

    g_garage_gauche.old = 2;
    g_garage_gauche.id = -1;

    g_garage_fenetre.old = 2;
    g_garage_fenetre.id = -1;

    g_garage_lumiere_etabli.old = 2000;
    g_garage_lumiere_etabli.state_old = 2;
    g_garage_lumiere_etabli.id = -1;

    g_garage_lumiere.old = 2000;
    g_garage_lumiere.state_old = 2;
    g_garage_lumiere.id = -1;

    g_garage_porte.old = 2;
    g_garage_porte.id = -1;

    g_bureau_porte.old = 2;
    g_bureau_porte.id = -1;

    g_bureau_fenetre.old = 2;
    g_bureau_fenetre.id = -1;

    g_cellier_porte_ext.old = 2;
    g_cellier_porte_ext.id = -1;

    g_cellier_porte.old = 2;
    g_cellier_porte.id = -1;

    g_cellier_lumiere.old = 2000;
    g_cellier_lumiere.state_old = 2;
    g_cellier_lumiere.id = -1;

    g_bureau_lumiere.old = 2000;
    g_bureau_lumiere.state_old = 2;
    g_bureau_lumiere.id = -1;

    g_lingerie_lumiere.old = 2000;
    g_lingerie_lumiere.state_old = 2;
    g_lingerie_lumiere.id = -1;

    g_lingerie_porte_cuisine.old = 2;
    g_lingerie_porte_cuisine.id = -1;

    g_lingerie_fenetre.old = 2;
    g_lingerie_fenetre.id = -1;

    g_entree_porte_ext.old = 2;
    g_entree_porte_ext.id = -1;

    g_cuisine_porte_ext.old = 2;
    g_cuisine_porte_ext.id = -1;

    g_poulailler_porte.old = 2;
    g_poulailler_porte.id = -1;

    g_poulailler.old = 2;
    g_poulailler.id = -1;

    g_poule_gauche.old = 2;
    g_poule_gauche.id = -1;

    g_poule_droite.old = 2;
    g_poule_droite.id = -1;

    g_temperature_garage = 0;
    g_temperature_bureau = 0;
    g_temperature_grenier = 0;

    g_grenier_lumiere.old = 2000;
    g_grenier_lumiere.state_old = 2;
    g_grenier_lumiere.id = -1;

    g_edf_hc.old = 999999;
    g_edf_hc.id = -1;
    EEPROM.get(EEPROM_ADDR_EDF_HC, g_edf_hc.value);

    g_edf_hp.old = 999999;
    g_edf_hp.id = -1;
    EEPROM.get(EEPROM_ADDR_EDF_HP, g_edf_hp.value);

    EEPROM.get(EEPROM_ADDR_EDF_WEEK_HC, g_edf_week_hc);
    EEPROM.get(EEPROM_ADDR_EDF_WEEK_HP, g_edf_week_hp);

    /* Init analog parts */
    value = analogRead(PIN_TEMP_EXT);
    value_offset = analogRead(PIN_TEMP_EXT_OFFSET);
    g_temperature_ext = ((500.0 * (value - value_offset)) / 1024) - OFFSET_TEMP_EXT;

    value = analogRead(PIN_TEMP_GARAGE);
    g_temperature_garage = (500.0 * value) / 1024;

    value = analogRead(PIN_TEMP_BUREAU);
    g_temperature_bureau  = ((500.0 * value) / 1024) + OFFSET_TEMP_BUREAU;

    value = analogRead(PIN_TEMP_GRENIER);
    g_temperature_grenier  = ((500.0 * value) / 1024) + OFFSET_TEMP_GRENIER;

    g_temperature_daymin = g_temperature_ext;
    g_temperature_daymax = g_temperature_ext;

    /* reset values */
    /* EEPROM.put(EEPROM_ADDR_MINYEAR, g_temperature_yearmin); */
    /* EEPROM.put(EEPROM_ADDR_MAXYEAR, g_temperature_yearmax); */


    EEPROM.get(EEPROM_ADDR_MINYEAR, g_temperature_yearmin);
    EEPROM.get(EEPROM_ADDR_MINYEAR_HOU, g_temperature_yearmin_hour);
    EEPROM.get(EEPROM_ADDR_MINYEAR_MIN, g_temperature_yearmin_min);
    EEPROM.get(EEPROM_ADDR_MINYEAR_DAY, g_temperature_yearmin_day);
    EEPROM.get(EEPROM_ADDR_MINYEAR_MON, g_temperature_yearmin_mon);
    sprintf(g_tempyearmin_string,"%d°C le %02d/%02d à %02dh%02d ",g_temperature_yearmin, g_temperature_yearmin_day, g_temperature_yearmin_mon, g_temperature_yearmin_hour, g_temperature_yearmin_min);

    EEPROM.get(EEPROM_ADDR_MAXYEAR, g_temperature_yearmax);
    EEPROM.get(EEPROM_ADDR_MAXYEAR_HOU, g_temperature_yearmax_hour);
    EEPROM.get(EEPROM_ADDR_MAXYEAR_MIN, g_temperature_yearmax_min);
    EEPROM.get(EEPROM_ADDR_MAXYEAR_DAY, g_temperature_yearmax_day);
    EEPROM.get(EEPROM_ADDR_MAXYEAR_MON, g_temperature_yearmax_mon);
    sprintf(g_tempyearmax_string,"%d°C le %02d/%02d à %02dh%02d",g_temperature_yearmax, g_temperature_yearmax_day, g_temperature_yearmax_mon, g_temperature_yearmax_hour, g_temperature_yearmax_min);

    EEPROM.get(EEPROM_ADDR_WEEK, g_week);

    /* reset values */
    /* EEPROM.put(EEPROM_ADDR_PLUVIO_MAXYEAR, 0); */
    /* EEPROM.put(EEPROM_ADDR_PLUVIO_MAXYEAR_DAY, 1); */
    /* EEPROM.put(EEPROM_ADDR_PLUVIO_MAXYEAR_MON, 1); */

    EEPROM.get(EEPROM_ADDR_PLUVIO_MAXYEAR, g_pluvio_max_cpt);
    EEPROM.get(EEPROM_ADDR_PLUVIO_MAXYEAR_DAY, g_pluvio_max_cpt_day);
    EEPROM.get(EEPROM_ADDR_PLUVIO_MAXYEAR_MON, g_pluvio_max_cpt_mon);
    pluvio = g_pluvio_max_cpt * PLUVIO_UNIT;
    sprintf(g_pluvio_max_string,"%d mm le %02d/%02d", pluvio, g_pluvio_max_cpt_day, g_pluvio_max_cpt_mon);

    /* reset values */
    /* EEPROM.put(EEPROM_ADDR_ANEMO_MAXYEAR, 0); */
    /* EEPROM.put(EEPROM_ADDR_ANEMO_MAXYEAR_HOU, 0); */
    /* EEPROM.put(EEPROM_ADDR_ANEMO_MAXYEAR_MIN, 0); */
    /* EEPROM.put(EEPROM_ADDR_ANEMO_MAXYEAR_DAY, 1); */
    /* EEPROM.put(EEPROM_ADDR_ANEMO_MAXYEAR_MON, 1); */

    EEPROM.get(EEPROM_ADDR_ANEMO_MAXYEAR, g_anemo_max_year_cpt);
    EEPROM.get(EEPROM_ADDR_ANEMO_MAXYEAR_HOU, g_anemo_max_year_cpt_hour);
    EEPROM.get(EEPROM_ADDR_ANEMO_MAXYEAR_MIN, g_anemo_max_year_cpt_min);
    EEPROM.get(EEPROM_ADDR_ANEMO_MAXYEAR_DAY, g_anemo_max_year_cpt_day);
    EEPROM.get(EEPROM_ADDR_ANEMO_MAXYEAR_MON, g_anemo_max_year_cpt_mon);
    sprintf(g_anemo_max_year_string,"%d.%d km/h le %02d/%02d à %02dh%02d",
	(uint16_t)(g_anemo_max_year_cpt * ANEMO_UNIT_SEC),
	(uint16_t)((g_anemo_max_year_cpt * ANEMO_10_SEC)%10),
	g_anemo_max_year_cpt_day, g_anemo_max_year_cpt_mon,
	g_anemo_max_year_cpt_hour, g_anemo_max_year_cpt_min);


    sprintf(g_girouette_string,"------");
    sprintf(g_pluvio_string,"0 mm");
    sprintf(g_pluvio_night_string,"0 mm");
    sprintf(g_anemo_string,"0 km/h");
    sprintf(g_anemo_max_day_string,"0 km/h");

    g_sched_temperature = 0;
    g_sched_door_already_opened = 0;
    g_sched_door_already_closed = 0;
    g_sched_midnight = 0;
    g_sched_night = 0;
    g_sched_temp_year = 0;
    g_sched_daymin_sms = 0;

    g_temperature_grenier_cpt = 0;
    g_temperature_grenier_total = 0;
    g_temperature_ext_cpt = 0;
    g_temperature_ext_total = 0;
    g_temperature_bureau_cpt = 0;
    g_temperature_bureau_total = 0;
    g_temperature_garage_cpt = 0;
    g_temperature_garage_total = 0;

    for(i = 0; i < NB_ITEM; i++ )
    {
	g_data_item[i].item = 0;
	g_data_item[i].date[0] = 0;
	g_data_item[i].hour[0] = 0;
	g_data_item[i].state[0] = 0;
	g_data_item[i].clas[0] = 0;
    }

    for(i = 0; i < NB_DELAY_MAX; i++)
    {
	g_delay[i].delay_inuse  = 0;
    }

    /* Initialisation de l'interruption INT0 (comptage pluviometre) */
    attachInterrupt(digitalPinToInterrupt(PIN_METEO_PLUVIOMETRE), interrupt_pluvio, RISING);

    /* Initialisation de l'interruption INT1 (comptage anemometre) */
    attachInterrupt(digitalPinToInterrupt(PIN_METEO_ANEMOMETRE), interrupt_anemo, RISING);

#ifdef DEBUG_MEM
    PgmPrint("Free RAM: ");
    Serial.println(FreeRam());

    PgmPrintln("EEPROM :");
    for(i=0;i<40;i++)
    {
	EEPROM.get(i, hour);

	sprintf(eeprom_str,"%02d:%02d", i, hour);
	Serial.println(eeprom_str);
    }
#endif

#ifdef DEBUG_SERIAL
    PgmPrintln("Init OK");
#endif
}

void save_eeprom()
{
    EEPROM.put(EEPROM_ADDR_EDF_HC, g_edf_hc.value);
    EEPROM.put(EEPROM_ADDR_EDF_HP, g_edf_hp.value);
    EEPROM.put(EEPROM_ADDR_TIMEZONE, g_timezone);

    EEPROM.put(EEPROM_ADDR_EDF_WEEK_HC, g_edf_week_hc);
    EEPROM.put(EEPROM_ADDR_EDF_WEEK_HP, g_edf_week_hp);
    EEPROM.put(EEPROM_ADDR_WEEK, g_week);

    EEPROM.put(EEPROM_ADDR_PLUVIO_MAXYEAR, g_pluvio_max_cpt);
    EEPROM.put(EEPROM_ADDR_PLUVIO_MAXYEAR_DAY, g_pluvio_max_cpt_day);
    EEPROM.put(EEPROM_ADDR_PLUVIO_MAXYEAR_MON, g_pluvio_max_cpt_mon);

    EEPROM.put(EEPROM_ADDR_ANEMO_MAXYEAR, g_anemo_max_year_cpt);
    EEPROM.put(EEPROM_ADDR_ANEMO_MAXYEAR_HOU, g_anemo_max_year_cpt_hour);
    EEPROM.put(EEPROM_ADDR_ANEMO_MAXYEAR_MIN, g_anemo_max_year_cpt_min);
    EEPROM.put(EEPROM_ADDR_ANEMO_MAXYEAR_DAY, g_anemo_max_year_cpt_day);
    EEPROM.put(EEPROM_ADDR_ANEMO_MAXYEAR_MON, g_anemo_max_year_cpt_mon);

    EEPROM.put(EEPROM_ADDR_MINYEAR, g_temperature_yearmin);
    EEPROM.put(EEPROM_ADDR_MINYEAR_HOU, g_temperature_yearmin_hour);
    EEPROM.put(EEPROM_ADDR_MINYEAR_MIN, g_temperature_yearmin_min);
    EEPROM.put(EEPROM_ADDR_MINYEAR_DAY, g_temperature_yearmin_day);
    EEPROM.put(EEPROM_ADDR_MINYEAR_MON, g_temperature_yearmin_mon);

    EEPROM.put(EEPROM_ADDR_MAXYEAR, g_temperature_yearmax);
    EEPROM.put(EEPROM_ADDR_MAXYEAR_HOU, g_temperature_yearmax_hour);
    EEPROM.put(EEPROM_ADDR_MAXYEAR_MIN, g_temperature_yearmax_min);
    EEPROM.put(EEPROM_ADDR_MAXYEAR_DAY, g_temperature_yearmax_day);
    EEPROM.put(EEPROM_ADDR_MAXYEAR_MON, g_temperature_yearmax_mon);
}

void send_gsm(uint8_t cmd, uint8_t *buffer, uint8_t size)
{
    uint8_t crc;
    uint8_t recv_crc;
    uint8_t nb_retry;
    uint8_t i;
    uint16_t nb_wait;

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

/** Store a string in flash memory.*/
#define send_SMS(x) send_SMS_P(PSTR(x), 0)
#define send_SMS_alerte(x) send_SMS_P(PSTR(x), 1)

void send_SMS_P(PGM_P str, uint8_t alerte)
{
    uint8_t i;
    uint8_t len;

    if (g_init_gsm == 0)
  	return;

    if ((alerte == 1) && (g_critical_alertes == 0))
	return;

    len = strlen_P(str);
    if (len > 0)
    {
	if (len > CMD_DATA_MAX)
	    len = CMD_DATA_MAX-1;

	strncpy_P((char *)g_send_to_gsm, str, len);

	send_gsm(IO_GSM_COMMAND_SMS, g_send_to_gsm, len);
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
	default:
	break;
    }
}

void deal_with_code(File *file, char item, char type, char code)
{
    code_t *ptr_code;
    char ipaddr[16];
    char id;

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
	case WEB_CODE_CHECKED:
	{
	    ptr_code = &g_code[TYPE_CHECKED];
	}break;
	case WEB_CODE_CRITICAL:
	{
	    ptr_code = &g_code[TYPE_CRITICAL];
	}break;
	case WEB_CODE_NOTCHECKED:
	{
	    ptr_code = &g_code[TYPE_NOTCHECKED];
	}break;
	default:
	{
	    ptr_code = &g_code[0];
	}break;
    }

    switch (item)
    {
	case 'a':
	{
	    sprintf(ipaddr,"%d.%d.%d.%d",g_remoteIP[0],g_remoteIP[1],g_remoteIP[2],g_remoteIP[3]);
	    g_client.print(ipaddr);
	}
	break;
	case 'b':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_lampe1],
		strlen(ptr_code->name[g_lampe1]));
	}
	break;
	case 'c':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_lampe2],
		strlen(ptr_code->name[g_lampe2]));
	}
	break;
	case 'd':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_lampe3],
		strlen(ptr_code->name[g_lampe3]));
	}
	break;
	case 'e':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_lampe4],
		strlen(ptr_code->name[g_lampe4]));
	}
	break;
	case 'f':
	{
	    g_client.print(g_edf_hc.value/1000);
	}break;
	case 'g':
	{
	    g_client.print(g_edf_hp.value/1000);
	}break;
	case 'h':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_timezone-1],
		strlen(ptr_code->name[g_timezone-1]));
	}
	break;
	case 'i':
	{
	    g_client.print(g_tempdaymin_string);
	}
	break;
	case 'j':
	{
	    g_client.print(g_tempdaymax_string);
	}
	break;
	case 'k':
	{
	    g_client.print(g_tempyearmin_string);
	}
	break;
	case 'l':
	{
	    g_client.print(g_tempyearmax_string);
	}
	break;
	case 'm':
	{
	    g_client.print((g_edf_hc.value - g_edf_week_hc)/1000);
	}
	break;
	case 'n':
	{
	    g_client.print((g_edf_hp.value - g_edf_week_hp)/1000);
	}
	break;
	case 'o':
	{
	    g_client.print(g_temperature_grenier);
	}break;
	case 'p':
	{
	    /* lumiere bureau */
	    if (g_debug)
	    {
		g_client.print(g_grenier_lumiere.curr);
	    }
	    else
	    {
		g_client.write((uint8_t*)ptr_code->name[g_grenier_lumiere.state_curr],
		    strlen(ptr_code->name[g_grenier_lumiere.state_curr]));
	    }
	}break;
	case 'q':
	{
	    if (g_debug)
	    {
		g_client.print(g_girouette);
	    }
	    else
	    {
		g_client.print(g_girouette_string);
	    }
	}break;
	case 'r':
	{
	    if (g_debug)
		g_client.print(g_anemo_cpt);
	    else
		g_client.print(g_anemo_string);
	}break;
	case 's':
	{
	    if (g_debug)
		g_client.print(g_pluvio_cpt);
	    else
	    {
		sprintf(g_pluvio_string,"%d mm", (uint16_t)(g_pluvio_cpt * PLUVIO_UNIT));
		g_client.print(g_pluvio_string);
	    }
	}break;
	case 't':
	{
		g_client.print(g_pluvio_night_string);
	}break;
	case 'u':
	{
		g_client.print(g_pluvio_max_string);
	}break;
	case ',':
	{
		g_client.print(g_anemo_max_day_string);
	}break;
	case '-':
	{
		g_client.print(g_anemo_max_year_string);
	}break;
	case 'v':
	{
	    if (g_NTP)
	    {
		g_client.print(g_start_date);
	    }
	}
	break;
	case 'w':
	{
	    if (g_critical_alertes)
	    {
		g_client.print(F("Systeme d'alertes : actif"));
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
	    if (g_NTP)
	    {
		g_client.print(g_tab_week[g_week]);
		g_client.print("  ");
		g_client.print(g_date);
		g_client.print(" ");
		g_client.print(g_clock);
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
	    g_client.print(g_temperature_ext);
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
	case 'Q':
	{
	    /* porte Bureau */
	    g_client.write((uint8_t*)ptr_code->name[g_bureau_porte.curr],
		strlen(ptr_code->name[g_bureau_porte.curr]));
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
	case 'T':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_poulailler.curr],
		strlen(ptr_code->name[g_poulailler.curr]));
	}break;
	case 'V':
	{
	    g_client.print(g_temperature_garage);
	}break;
	case 'W':
	{
	    /* then get the id */
	    id = file->read();

	    /* action lampe 4 */
	    if ((id == 'L') && (g_week == 1))
	    {
		g_client.write((uint8_t*)ptr_code->name[0],
		    strlen(ptr_code->name[0]));
	    }
	    else if ((id == 'M') && (g_week == 2))
	    {
		g_client.write((uint8_t*)ptr_code->name[0],
		    strlen(ptr_code->name[0]));
	    }
	    else if ((id == 'E') && (g_week == 3))
	    {
		g_client.write((uint8_t*)ptr_code->name[0],
		    strlen(ptr_code->name[0]));
	    }
	    else if ((id == 'J') && (g_week == 4))
	    {
		g_client.write((uint8_t*)ptr_code->name[0],
		    strlen(ptr_code->name[0]));
	    }
	    else if ((id == 'V') && (g_week == 5))
	    {
		g_client.write((uint8_t*)ptr_code->name[0],
		    strlen(ptr_code->name[0]));
	    }
	    else if ((id == 'S') && (g_week == 6))
	    {
		g_client.write((uint8_t*)ptr_code->name[0],
		    strlen(ptr_code->name[0]));
	    }
	    else if ((id == 'D') && ((g_week == 7) || (g_week == 0)))
	    {
		g_client.write((uint8_t*)ptr_code->name[0],
		    strlen(ptr_code->name[0]));
	    }
	    break;
	}
	case 'X':
	{
	    /* lumiere bureau */
	    if (g_debug)
	    {
		g_client.print(g_bureau_lumiere.curr);
	    }
	    else
	    {
		g_client.write((uint8_t*)ptr_code->name[g_bureau_lumiere.state_curr],
		    strlen(ptr_code->name[g_bureau_lumiere.state_curr]));
	    }
	}break;
	case 'Y':
	{
	    g_client.print(g_temperature_bureau);
	}break;
	case 'Z':
	{
	    /* fenetre bureau */
	    g_client.write((uint8_t*)ptr_code->name[g_bureau_fenetre.curr],
		strlen(ptr_code->name[g_bureau_fenetre.curr]));
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
		deal_with_code(file, item, type, code);
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

#ifdef DEBUG_SERIAL
void digitalClockDisplaySerial(void)
{
    /* save current date and clock in global var */
    digitalClock();
    digitalDate();
    Serial.print(g_clock);
    Serial.print("  ");
    Serial.print(g_tab_week[g_week]);
    Serial.print(" ");
    Serial.print(g_date);
    Serial.println();
}
#endif

void digitalClock(void)
{
    sprintf(g_clock,"%02d:%02d:%02d",g_hour, g_min, g_sec);
}

void digitalDate(void)
{
    sprintf(g_date,"%02d/%02d/%02d",g_day, g_mon, g_year-2000);
}

time_t getNtpTime(void)
{
    int size;
    unsigned long secsSince1900;

    /* discard any previously received packets */
    while (g_Udp.parsePacket() > 0);

#ifdef DEBUG_NTP
    Serial.println("Transmit NTP");
#endif

    sendNTPpacket(g_timeServer);

    g_beginWait = millis();
    while (millis() - g_beginWait < 1500)
    {
	size = g_Udp.parsePacket();

	if (size >= NTP_PACKET_SIZE)
	{

#ifdef DEBUG_NTP
	    Serial.println("Receive NTP");
#endif

	    g_Udp.read(g_packetBuffer, NTP_PACKET_SIZE);

	    /* NTP is ok and running */
	    g_NTP = 1;

	    g_process_schedule = PROCESS_ON;

	    /* convert four bytes starting at location 40 to a long integer */
	    secsSince1900 =  (unsigned long)g_packetBuffer[40] << 24;
	    secsSince1900 |= (unsigned long)g_packetBuffer[41] << 16;
	    secsSince1900 |= (unsigned long)g_packetBuffer[42] << 8;
	    secsSince1900 |= (unsigned long)g_packetBuffer[43];
	    return secsSince1900 - 2208988800UL + g_timezone * SECS_PER_HOUR;
	}
    }
#ifdef DEBUG_NTP
    Serial.println("No NTP :-(");
#endif
    return 0;
}

/* send an NTP request to the time server at the given address */
void sendNTPpacket(char *address)
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

#ifdef DEBUG_SERIAL
void process_serial(void)
{
    uint16_t count;
    uint8_t command;

    if (g_process_serial != PROCESS_OFF)
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
    uint8_t recv_size;
    uint8_t recv_index;
    uint8_t error;
    uint8_t command;
    uint16_t nb_wait;

    if (g_process_recv_gsm == PROCESS_RECV_GSM_WAIT_COMMAND)
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
				    g_recv_gsm[recv_index] = Serial.read();
				    crc += g_recv_gsm[recv_index];
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
				    g_recv_gsm[recv_index] = 0;

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
				g_process_recv_gsm = PROCESS_RECV_GSM_DO_NOTHING;
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

void process_ethernet(void)
{
    uint8_t count;
    char *end_filename;
    char *paramstr;
    char *end_paramstr;
    char eat_req;
    char save_char;

    int exit;

    if (g_process_ethernet != PROCESS_OFF)
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
	    /* Get remote IP */
	    g_client.getRemoteIP(g_remoteIP);

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
			/* GET /config.htm?week=jeu HTTP/1.1 */
			/* Check if it's a request for */
			if (strstr(g_line,"?week") != NULL)
			{
			    /* check for parameters */
			    if (strstr(g_line,"lun") != NULL)
			    {
				g_week = 1;
			    }
			    else if (strstr(g_line,"mar") != NULL)
			    {
				g_week = 2;
			    }
			    else if (strstr(g_line,"mer") != NULL)
			    {
				g_week = 3;
			    }
			    else if (strstr(g_line,"jeu") != NULL)
			    {
				g_week = 4;
			    }
			    else if (strstr(g_line,"ven") != NULL)
			    {
				g_week = 5;
			    }
			    else if (strstr(g_line,"sam") != NULL)
			    {
				g_week = 6;
			    }
			    else if (strstr(g_line,"dim") != NULL)
			    {
				g_week = 7;
			    }
			}
			else if (strstr(g_line,"?Lampe") != NULL)
			{
			    /* GET /config.htm?Lampe3=on&Lampe4=on HTTP/1.1 */
			    /* Check if it's a request for */
			    /* check for parameters */
			    paramstr = strstr(g_line,"lampe1_on");
			    if (paramstr != NULL)
			    {
				g_lampe1 = LAMPE_ON;
				g_process_action = PROCESS_ACTION_LAMPE;
			    }
			    else
			    {
				g_lampe1 = LAMPE_OFF;
				g_process_action = PROCESS_ACTION_LAMPE;
			    }

			    paramstr = strstr(g_line,"lampe2_on");
			    if (paramstr != NULL)
			    {
				g_lampe2 = LAMPE_ON;
				g_process_action = PROCESS_ACTION_LAMPE;
			    }
			    else
			    {
				g_lampe2 = LAMPE_OFF;
				g_process_action = PROCESS_ACTION_LAMPE;
			    }

			    paramstr = strstr(g_line,"lampe3_on");
			    if (paramstr != NULL)
			    {
				g_lampe3 = LAMPE_ON;
				g_process_action = PROCESS_ACTION_LAMPE;
			    }
			    else
			    {
				g_lampe3 = LAMPE_OFF;
				g_process_action = PROCESS_ACTION_LAMPE;
			    }

			    paramstr = strstr(g_line,"lampe4_on");
			    if (paramstr != NULL)
			    {
				g_lampe4 = LAMPE_ON;
				g_process_action = PROCESS_ACTION_LAMPE;
			    }
			    else
			    {
				g_lampe4 = LAMPE_OFF;
				g_process_action = PROCESS_ACTION_LAMPE;
			    }
			}
			else if (strstr(g_line,"?h") != NULL )
			{
			    /* Check if it's a request for edf */
			    paramstr = strstr(g_line,"hc=");
			    if (paramstr != NULL)
			    {
				end_paramstr = strstr(g_line,"&");
				if (end_paramstr != NULL)
				{
				    save_char = *end_paramstr;
				    *end_paramstr = '\0';

				    g_edf_hc.value = strtoul(paramstr+3, NULL, 10) * 1000;
				    g_process_action = PROCESS_ACTION_EDF;
				    *end_paramstr = save_char;
				}
			    }
			    paramstr = strstr(g_line,"hp=");
			    if (paramstr != NULL)
			    {
				end_paramstr = strstr(g_line," ");
				if (end_paramstr != NULL)
				{
				    save_char = *end_paramstr;
				    *end_paramstr = '\0';

				    g_edf_hp.value   = strtoul(paramstr+3, NULL, 10) * 1000;
				    g_process_action = PROCESS_ACTION_EDF;
				    *end_paramstr    = save_char;
				}
			    }
			}
			else if (strstr(g_line,"?t") != NULL )
			{
			    /* Check if it's a request for NTP */
			    paramstr = strstr(g_line,"time1");
			    if (paramstr != NULL)
			    {
				g_timezone = 1;
				g_process_action = PROCESS_ACTION_TIMEZONE;
			    }
			    else
			    {
				g_timezone = 2;
				g_process_action = PROCESS_ACTION_TIMEZONE;
			    }
			}
			/* Now get the filename to load the page */
			end_filename = strstr(g_line,"?");
			if (end_filename == NULL)
			{
			    end_filename = strstr(g_line," HTTP");
			}

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

		    /* check if filename is a keyword */
		    if (strcmp(g_filename.name, "temp_ext") == 0)
		    {
			g_client.print(g_temperature_ext);
		    }
		    else if (strcmp(g_filename.name, "garage") == 0)
		    {
			if (g_garage_porte.curr == 0)
			{
			    g_client.println(F("ouverte"));
			}
			else
			{
			    g_client.println(F("fermer"));
			}
		    }
		    else
		    {
			/* must be a web page to serve */
			if (g_filename.fd.available())
			{
			    if ((strstr(g_filename.name, "config.htm") != NULL) &&
				(g_remoteIP[0] != 192) &&
				(g_remoteIP[1] != 168) &&
				(g_remoteIP[2] != 5))
			    {
				g_client.println(F("HTTP/1.1 404 Not Authorized"));
				g_client.println(F("Content-Type: text/html"));
				g_client.println();
				g_client.println(F("<h2>Domotix Error: You are not allowed to open this page!</h2>"));
			    }
			    else
			    {
				/* send 200 OK */
				g_client.println(F("HTTP/1.1 200 OK"));

				if (strstr(g_filename.name, ".htm") != NULL)
				{
				    g_client.println(F("Content-Type: text/html"));

				    /* end of header */
				    g_client.println();
				    send_file_to_client(&g_filename.fd);
				    g_filename.fd.close();
				}
				else
				{
				    if (strstr(g_filename.name, ".css") != NULL)
				    {
					g_client.println(F("Content-Type: text/css"));
				    }
				    else if (strstr(g_filename.name, ".jpg") != NULL)
				    {
					g_client.println(F("Content-Type: image/jpeg"));
					g_client.println(F("Cache-Control: max-age=2592000"));
				    }
				    else if (strstr(g_filename.name, ".ico") != NULL)
				    {
					g_client.println(F("Content-Type: image/x-icon"));
					g_client.println(F("Cache-Control: max-age=2592000"));
				    }
				    else
					g_client.println(F("Content-Type: text"));

				    /* end of header */
				    g_client.println();

				    send_resp_to_client(&g_filename.fd);
				    g_filename.fd.close();
				}
			    }
			}
			else
			{
			    g_client.println(F("HTTP/1.1 404 Not Found"));
			    g_client.println(F("Content-Type: text/html"));
			    g_client.println();
			    g_client.println(F("<h2>Domotix Error: File Not Found!</h2>"));
			}
		    }
		}
		else
		{
		    g_client.println(F("HTTP/1.1 404 Not Found"));
		    g_client.println(F("Content-Type: text/html"));
		    g_client.println();
		    g_client.println(F("<h2>Domotix Error: GET /  Not Found!</h2>"));
		}
	    }
	    else
	    {
		g_client.println(F("HTTP/1.1 404 Not Found"));
		g_client.println(F("Content-Type: text/html"));
		g_client.println();
		g_client.println(F("<h2>Domotix Error: GET /  Error, line too long!</h2>"));
	    }
	    /* close connection */
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

void save_entry_string(const char *file, char *string1, char *string2)
{
    File  fd;
    char data[4+1];

    /* write in file
     * Format :
     * 12/12/2014
     * 09:35:42
     * string
     */

    /* Date must be ready to save entry */
    if (g_NTP == 0)
	return;

    /* check if file is too big, then save it */
    copy_file(file);

    fd = SD.open(file, FILE_WRITE);

    fd.println(SEPARATE_ITEM);
    fd.println(g_date);
    fd.println(string1);
    fd.println(string2);
    fd.println("ok");
    fd.close();
}

void process_domotix(void)
{
    int16_t value;
    int16_t value_offset;
    int16_t temp;

    if (g_process_domotix != PROCESS_OFF)
    {
	g_garage_droite.curr =  digitalRead(PIN_GARAGE_DROITE);
	if (g_garage_droite.curr != g_garage_droite.old)
	{
	    g_garage_droite.old = g_garage_droite.curr;

	    /* write in file  */
	    save_entry("A.txt", g_garage_droite.curr, TYPE_PORTE);

	    /* Critical part */
	    if (g_garage_droite.curr == 0)
	    {
		/* Send SMS */
		send_SMS_alerte("La porte de droite du garage vient de s'ouvrir");
	    }
	    else
	    {
		send_SMS_alerte("La porte de droite du garage vient de se fermer");
	    }
	}

	g_garage_gauche.curr =  digitalRead(PIN_GARAGE_GAUCHE);
	if (g_garage_gauche.curr != g_garage_gauche.old)
	{
	    g_garage_gauche.old = g_garage_gauche.curr;

	    /* write in file  */
	    save_entry("B.txt", g_garage_gauche.curr, TYPE_PORTE);

	    /* Critical part */
	    if (g_garage_gauche.curr == 0)
	    {
		/* Send SMS */
		send_SMS_alerte("La porte de gauche du garage vient de s'ouvrir");
	    }
	    else
	    {
		send_SMS_alerte("La porte de gauche du garage vient de se fermer");
	    }
	}

	g_garage_fenetre.curr =  digitalRead(PIN_GARAGE_FENETRE);
	if (g_garage_fenetre.curr != g_garage_fenetre.old)
	{
	    g_garage_fenetre.old = g_garage_fenetre.curr;

	    /* write in file  */
	    save_entry("C.txt", g_garage_fenetre.curr, TYPE_PORTE);

	    /* Critical part */
	    if (g_garage_fenetre.curr == 0)
	    {
		/* Send SMS */
		send_SMS_alerte("La fenetre du garage vient de s'ouvrir");
	    }
	    else
	    {
		send_SMS_alerte("La fenetre du garage vient de se fermer");
	    }
	}

	g_cellier_porte_ext.curr =  digitalRead(PIN_CELLIER_PORTE_EXT);
	if (g_cellier_porte_ext.curr != g_cellier_porte_ext.old)
	{
	    g_cellier_porte_ext.old = g_cellier_porte_ext.curr;

	    /* write in file  */
	    save_entry("E.txt", g_cellier_porte_ext.curr, TYPE_PORTE);

	    /* Critical part */
	    if (g_cellier_porte_ext.curr == 0)
	    {
		/* Send SMS */
		send_SMS_alerte("La porte exterieure du cellier vient de s'ouvrir");

		/* Arm event to avoid openning the door too long */
		/* 5min maxi 5*60*1000 = 300000*/
		g_evt_cellier_porte_ext_open.timeout = 300000;
		event_add(&g_evt_cellier_porte_ext_open, callback_wait_portecellier);

		g_evt_buzz_before5min_on.timeout = 1000;
		event_add(&g_evt_buzz_before5min_on, callback_buzz_portecellier_on);
	    }
	    else
	    {
		event_del(&g_evt_cellier_porte_ext_open);
		event_del(&g_evt_buzz_before5min_off);
		event_del(&g_evt_buzz_before5min_on);
		analogWrite(PIN_OUT_BUZZER, 0);
		send_SMS_alerte("La porte exterieure du cellier vient de se fermer");
	    }
	}

	g_cellier_porte.curr =  digitalRead(PIN_CELLIER_PORTE_INT);
	if (g_cellier_porte.curr != g_cellier_porte.old)
	{
	    g_cellier_porte.old = g_cellier_porte.curr;

	    /* write in file  */
	    save_entry("F.txt", g_cellier_porte.curr, TYPE_PORTE);
	}

	g_lingerie_porte_cuisine.curr =  digitalRead(PIN_LINGERIE_CUISINE);
	if (g_lingerie_porte_cuisine.curr != g_lingerie_porte_cuisine.old)
	{
	    g_lingerie_porte_cuisine.old = g_lingerie_porte_cuisine.curr;

	    /* write in file  */
	    save_entry("K.txt", g_lingerie_porte_cuisine.curr, TYPE_PORTE);
	}

	g_garage_porte.curr =  digitalRead(PIN_GARAGE_FOND);
	if (g_garage_porte.curr != g_garage_porte.old)
	{
	    g_garage_porte.old = g_garage_porte.curr;

	    /* write in file  */
	    save_entry("H.txt", g_garage_porte.curr, TYPE_PORTE);
	}

	g_bureau_porte.curr =  digitalRead(PIN_BUREAU_PORTE);
	if (g_bureau_porte.curr != g_bureau_porte.old)
	{
	    g_bureau_porte.old = g_bureau_porte.curr;

	    /* write in file  */
	    save_entry("Q.txt", g_bureau_porte.curr, TYPE_PORTE);
	}

	g_bureau_fenetre.curr = digitalRead(PIN_BUREAU_FENETRE);
	if (g_bureau_fenetre.curr != g_bureau_fenetre.old)
	{
	    g_bureau_fenetre.old = g_bureau_fenetre.curr;

	    /* write in file  */
	    save_entry("Z.txt", g_bureau_fenetre.curr, TYPE_PORTE);

	    /* Critical part */
	    if (g_bureau_fenetre.curr == 0)
	    {
		/* Send SMS */
		send_SMS_alerte("La Fenetre du bureau vient de s'ouvrir");
	    }
	    else
	    {
		send_SMS_alerte("La Fenetre du bureau vient de se fermer");
	    }
	}

	g_cuisine_porte_ext.curr = digitalRead(PIN_CUISINE_EXT);
	if (g_cuisine_porte_ext.curr != g_cuisine_porte_ext.old)
	{
	    g_cuisine_porte_ext.old = g_cuisine_porte_ext.curr;

	    /* write in file  */
	    save_entry("L.txt", g_cuisine_porte_ext.curr, TYPE_PORTE);

	    /* Critical part */
	    if (g_cuisine_porte_ext.curr == 0)
	    {
		/* Send SMS */
		send_SMS_alerte("La porte exterieure de la cuisine vient de s'ouvrir");
	    }
	    else
	    {
		/* Send SMS */
		send_SMS_alerte("La porte exterieure de la cuisine vient de se fermer");
	    }
	}

	g_lingerie_fenetre.curr = digitalRead(PIN_LINGERIE_FENETRE);
	if (g_lingerie_fenetre.curr != g_lingerie_fenetre.old)
	{
	    g_lingerie_fenetre.old = g_lingerie_fenetre.curr;

	    /* write in file  */
	    save_entry("N.txt", g_lingerie_fenetre.curr, TYPE_PORTE);

	    /* Critical part */
	    if (g_lingerie_fenetre.curr == 0)
	    {
		/* Send SMS */
		send_SMS_alerte("La Fenetre de la lingerie vient de s'ouvrir");
	    }
	    else
	    {
		send_SMS_alerte("La Fenetre de la lingerie vient de se fermer");
	    }
	}

	g_entree_porte_ext.curr = digitalRead(PIN_ENTREE_PORTE_EXT);
	if (g_entree_porte_ext.curr != g_entree_porte_ext.old)
	{
	    g_entree_porte_ext.old = g_entree_porte_ext.curr;

	    /* write in file  */
	    save_entry("O.txt", g_entree_porte_ext.curr, TYPE_PORTE);

	    /* Critical part */
	    if (g_entree_porte_ext.curr == 0)
	    {
		if (g_garage_droite.curr == 0)
		{
		    /* Send SMS */
		    send_SMS("Attention, la porte de droite du garage est ouverte");
		}
		else if (g_garage_gauche.curr == 0)
		{
		    /* Send SMS */
		    send_SMS("Attention, la porte de gauche du garage est ouverte");
		}
		else if (g_garage_fenetre.curr == 0)
		{
		    /* Send SMS */
		    send_SMS("Attention, la fenetre du garage est ouverte");
		}
		else if (g_cellier_porte_ext.curr == 0)
		{
		    /* Send SMS */
		    send_SMS("Attention, la porte du cellier est ouverte");
		}
		else if (g_bureau_fenetre.curr == 0)
		{
		    /* Send SMS */
		    send_SMS("Attention, la fenetre du bureau est ouverte");
		}
		else if (g_cuisine_porte_ext.curr == 0)
		{
		    /* Send SMS */
		    send_SMS("Attention, la porte de la cuisine est ouverte");
		}
		else if (g_lingerie_fenetre.curr == 0)
		{
		    /* Send SMS */
		    send_SMS("Attention, la fenetre de la lingerie est ouverte");
		}
		else
		{
		    send_SMS_alerte("La porte d'entree vient de s'ouvrir");
		}
	    }
	    else
		send_SMS_alerte("La porte d'entree vient de se fermer");
	}

	g_poulailler_porte.curr = digitalRead(PIN_POULAILLER_PORTE);
	if (g_poulailler_porte.curr != g_poulailler_porte.old)
	{
	    g_poulailler_porte.old = g_poulailler_porte.curr;

	    /* write in file  */
	    save_entry("P.txt", g_poulailler_porte.curr, TYPE_PORTE);
	}

	g_poule_gauche.curr = digitalRead(PIN_POULE_GAUCHE);
	if (g_poule_gauche.curr != g_poule_gauche.old)
	{
	    g_poule_gauche.old = g_poule_gauche.curr;

	    /* write in file  */
	    save_entry("R.txt", g_poule_gauche.curr, TYPE_POULE);
	}

	g_poule_droite.curr = digitalRead(PIN_POULE_DROITE);
	if (g_poule_droite.curr != g_poule_droite.old)
	{
	    g_poule_droite.old = g_poule_droite.curr;

	    /* write in file  */
	    save_entry("S.txt", g_poule_droite.curr, TYPE_POULE);
	}

	g_poulailler.curr = digitalRead(PIN_POULAILLER_HALL);
	if (g_poulailler.curr != g_poulailler.old)
	{
	    g_poulailler.old = g_poulailler.curr;

	    /* write in file  */
	    save_entry("T.txt", g_poulailler.curr, TYPE_POULE);
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
	if ((g_garage_lumiere_etabli.curr > (g_garage_lumiere_etabli.old + THRESHOLD_CMP_OLD)) ||
	    ((g_garage_lumiere_etabli.curr + THRESHOLD_CMP_OLD) < g_garage_lumiere_etabli.old))
	{
	    g_garage_lumiere_etabli.old = g_garage_lumiere_etabli.curr;

	    if (g_garage_lumiere_etabli.curr > THRESHOLD_LIGHT_ON_ETABLI)
	    {
		g_garage_lumiere_etabli.state_curr = 0;
	    }
	    else
	    {
		g_garage_lumiere_etabli.state_curr = 1;
	    }

	    if (g_garage_lumiere_etabli.state_curr != g_garage_lumiere_etabli.state_old)
	    {
		g_garage_lumiere_etabli.state_old = g_garage_lumiere_etabli.state_curr;

		/* write in file  */
		save_entry("D.txt", g_garage_lumiere_etabli.state_curr, TYPE_LUMIERE);
	    }
	}

	g_bureau_lumiere.curr =  analogRead(PIN_BUREAU_LUMIERE);
	if ( (g_bureau_lumiere.curr > (g_bureau_lumiere.old + THRESHOLD_CMP_OLD)) ||
	    ((g_bureau_lumiere.curr + THRESHOLD_CMP_OLD) < g_bureau_lumiere.old) )
	{
	    g_bureau_lumiere.old = g_bureau_lumiere.curr;

	    if (g_bureau_lumiere.curr > THRESHOLD_LIGHT_ON_BUREAU)
	    {
		g_bureau_lumiere.state_curr = 0;
	    }
	    else
	    {
		g_bureau_lumiere.state_curr = 1;
	    }

	    if (g_bureau_lumiere.state_curr != g_bureau_lumiere.state_old)
	    {
		g_bureau_lumiere.state_old = g_bureau_lumiere.state_curr;

		/* write in file  */
		save_entry("X.txt", g_bureau_lumiere.state_curr, TYPE_LUMIERE);
	    }
	}

	g_cellier_lumiere.curr =  analogRead(PIN_CELLIER_LUMIERE);
	if ( (g_cellier_lumiere.curr > (g_cellier_lumiere.old + THRESHOLD_CMP_OLD)) ||
	    ((g_cellier_lumiere.curr + THRESHOLD_CMP_OLD) < g_cellier_lumiere.old) )
	{
	    g_cellier_lumiere.old = g_cellier_lumiere.curr;

	    if (g_cellier_lumiere.curr > THRESHOLD_LIGHT_ON_CELLIER)
	    {
		g_cellier_lumiere.state_curr = 0;
	    }
	    else
	    {
		g_cellier_lumiere.state_curr = 1;
	    }

	    if (g_cellier_lumiere.state_curr != g_cellier_lumiere.state_old)
	    {
		g_cellier_lumiere.state_old = g_cellier_lumiere.state_curr;

		/* write in file  */
		save_entry("G.txt", g_cellier_lumiere.state_curr, TYPE_LUMIERE);
	    }
	}

	g_garage_lumiere.curr =  analogRead(PIN_GARAGE_LUMIERE);
	if ( (g_garage_lumiere.curr > (g_garage_lumiere.old + THRESHOLD_CMP_OLD)) ||
	    ((g_garage_lumiere.curr + THRESHOLD_CMP_OLD) < g_garage_lumiere.old) )
	{
	    g_garage_lumiere.old = g_garage_lumiere.curr;

	    if (g_garage_lumiere.curr > (THRESHOLD_LIGHT_ON_GARAGE))
	    {
		g_garage_lumiere.state_curr = 0;
	    }
	    else
	    {
		g_garage_lumiere.state_curr = 1;
	    }

	    if (g_garage_lumiere.state_curr != g_garage_lumiere.state_old)
	    {
		g_garage_lumiere.state_old = g_garage_lumiere.state_curr;

		/* write in file  */
		save_entry("I.txt", g_garage_lumiere.state_curr, TYPE_LUMIERE);
	    }
	}

	g_lingerie_lumiere.curr =  analogRead(PIN_LINGERIE_LUMIERE);
	if ( (g_lingerie_lumiere.curr > (g_lingerie_lumiere.old + THRESHOLD_CMP_OLD)) ||
	    ((g_lingerie_lumiere.curr + THRESHOLD_CMP_OLD) < g_lingerie_lumiere.old) )
	{
	    g_lingerie_lumiere.old = g_lingerie_lumiere.curr;

	    if (g_lingerie_lumiere.curr > THRESHOLD_LIGHT_ON_LINGERIE)
	    {
		g_lingerie_lumiere.state_curr = 0;
	    }
	    else
	    {
		g_lingerie_lumiere.state_curr = 1;
	    }

	    if (g_lingerie_lumiere.state_curr != g_lingerie_lumiere.state_old)
	    {
		g_lingerie_lumiere.state_old = g_lingerie_lumiere.state_curr;

		/* write in file  */
		save_entry("J.txt", g_lingerie_lumiere.state_curr, TYPE_LUMIERE);
	    }
	}

	g_grenier_lumiere.curr =  analogRead(PIN_GRENIER_LUMIERE);
	if ( (g_grenier_lumiere.curr > (g_grenier_lumiere.old + THRESHOLD_CMP_OLD)) ||
	    ((g_grenier_lumiere.curr + THRESHOLD_CMP_OLD) < g_grenier_lumiere.old) )
	{
	    g_grenier_lumiere.old = g_grenier_lumiere.curr;

	    if (g_grenier_lumiere.curr > THRESHOLD_LIGHT_ON_GRENIER)
	    {
		g_grenier_lumiere.state_curr = 0;
	    }
	    else
	    {
		g_grenier_lumiere.state_curr = 1;
	    }

	    if (g_grenier_lumiere.state_curr != g_grenier_lumiere.state_old)
	    {
		g_grenier_lumiere.state_old = g_grenier_lumiere.state_curr;

		/* write in file  */
		save_entry("oo.txt", g_grenier_lumiere.state_curr, TYPE_LUMIERE);
	    }
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

	value = analogRead(PIN_TEMP_GARAGE);
	temp  = (500.0 * value) / 1024;
	g_temperature_garage_cpt++;
	g_temperature_garage_total += temp;

	if (g_temperature_garage_cpt > PROCESS_DOMOTIX_CALC_CPT )
	{
	    g_temperature_garage = g_temperature_garage_total  / g_temperature_garage_cpt;
	    g_temperature_garage_total = 0;
	    g_temperature_garage_cpt = 0;
	}

	value = analogRead(PIN_TEMP_BUREAU);
	temp  = ((500.0 * value) / 1024) + OFFSET_TEMP_BUREAU;
	g_temperature_bureau_cpt++;
	g_temperature_bureau_total += temp;

	if (g_temperature_bureau_cpt >  PROCESS_DOMOTIX_CALC_CPT)
	{
	    g_temperature_bureau = g_temperature_bureau_total  / g_temperature_bureau_cpt;
	    g_temperature_bureau_total = 0;
	    g_temperature_bureau_cpt = 0;
	}


	value = analogRead(PIN_TEMP_GRENIER);
	temp  = ((500.0 * value) / 1024) + OFFSET_TEMP_GRENIER;
	g_temperature_grenier_cpt++;
	g_temperature_grenier_total += temp;

	if (g_temperature_grenier_cpt > PROCESS_DOMOTIX_CALC_CPT)
	{
	    g_temperature_grenier = g_temperature_grenier_total  / g_temperature_grenier_cpt;
	    g_temperature_grenier_total = 0;
	    g_temperature_grenier_cpt = 0;
	}

	value = analogRead(PIN_TEMP_EXT);
	value_offset = analogRead(PIN_TEMP_EXT_OFFSET);
	temp  = ((500.0 * (value - value_offset)) / 1024) - OFFSET_TEMP_EXT;
	g_temperature_ext_cpt++;
	g_temperature_ext_total += temp;

	if (g_temperature_ext_cpt > PROCESS_DOMOTIX_CALC_CPT)
	{
	    g_temperature_ext = g_temperature_ext_total  / g_temperature_ext_cpt;
	    g_temperature_ext_total = 0;
	    g_temperature_ext_cpt = 0;
	}

	value = analogRead(PIN_METEO_GIROUETTE);
	g_girouette_cpt++;
	g_girouette_total += value;
	if (g_girouette_cpt > PROCESS_DOMOTIX_CALC_CPT)
	{
	    g_girouette = g_girouette_total / g_girouette_cpt;
	    g_girouette_total = 0;
	    g_girouette_cpt = 0;


	    if (g_girouette > GIROUETTE_MAX_8)
	    {
		sprintf(g_girouette_string,"Nord Ouest");
	    }
	    else if (g_girouette > GIROUETTE_MAX_7)
	    {
		sprintf(g_girouette_string,"Nord");
	    }
	    else if (g_girouette > GIROUETTE_MAX_6)
	    {
		sprintf(g_girouette_string,"Nord Est");
	    }
	    else if (g_girouette > GIROUETTE_MAX_5)
	    {
		sprintf(g_girouette_string,"Ouest");
	    }
	    else if (g_girouette > GIROUETTE_MAX_4)
	    {
		sprintf(g_girouette_string,"Est");
	    }
	    else if (g_girouette > GIROUETTE_MAX_3)
	    {
		sprintf(g_girouette_string,"Sud Ouest");
	    }
	    else if (g_girouette > GIROUETTE_MAX_2)
	    {
		sprintf(g_girouette_string,"Sud");
	    }
	    else if (g_girouette > GIROUETTE_MAX_1)
	    {
		sprintf(g_girouette_string,"Sud Est");
	    }
	    else
	    {
		sprintf(g_girouette_string,"------");
	    }
	}

#ifdef DEBUG_TEMP
	Serial.println(g_clock);
	Serial.print("garage = ");
	Serial.print(g_temperature_garage);
	Serial.print("grenier = ");
	Serial.print(g_temperature_grenier);
	Serial.print(" ext = ");
	Serial.println(g_temperature_ext);
#endif

	if (g_temperature_ext <= g_temperature_daymin)
	{
	    g_temperature_daymin = g_temperature_ext;
	    sprintf(g_tempdaymin_string,"%d°C à %02dh%02d",g_temperature_daymin ,g_hour, g_min);
	}

	if (g_temperature_ext >= g_temperature_daymax)
	{
	    g_temperature_daymax = g_temperature_ext;
	    sprintf(g_tempdaymax_string,"%d°C à %02dh%02d",g_temperature_daymax ,g_hour, g_min);
	}

	if (g_temperature_ext <= g_temperature_yearmin)
	{
	    g_temperature_yearmin = g_temperature_ext;
	    sprintf(g_tempyearmin_string,"%d°C le %02d/%02d à %02dh%02d",g_temperature_yearmin, g_day, g_mon, g_hour, g_min);

	    /* save values for eeprom write à midnight */
	    g_temperature_yearmin_hour = g_hour;
	    g_temperature_yearmin_min = g_min;
	    g_temperature_yearmin_day = g_day;
	    g_temperature_yearmin_mon = g_mon;
	}

	if (g_temperature_ext >= g_temperature_yearmax)
	{
	    g_temperature_yearmax = g_temperature_ext;
	    sprintf(g_tempyearmax_string,"%d°C le %02d/%02d à %02dh%02d", g_temperature_yearmax, g_day, g_mon ,g_hour, g_min);

	    /* save values for eeprom write à midnight */
	    g_temperature_yearmax_hour = g_hour;
	    g_temperature_yearmax_min = g_min;
	    g_temperature_yearmax_day = g_day;
	    g_temperature_yearmax_mon = g_mon;
	}

	/* wait some time, before testing the next time the inputs */
	g_process_domotix = PROCESS_OFF;
	g_evt_process_domotix.timeout = PROCESS_DOMOTIX_TIMEOUT;
	event_add(&g_evt_process_domotix, callback_wait_pdomotix);
    }
}

void process_domotix_quick(void)
{
    state_lumiere_s *edf;

    if (g_process_domotix_quick != PROCESS_OFF)
    {
	/* Meteo every 1 sec */
	if ((millis() - g_beginWait1sec) >= 1000)
	{
	    g_beginWait1sec = millis();

	    sprintf(g_anemo_string,"%d.%d km/h",
		(uint16_t)(g_anemo_cpt * ANEMO_UNIT_SEC),
		(uint16_t)((g_anemo_cpt * ANEMO_10_SEC)%10));

	    if (g_anemo_cpt > g_anemo_max_day_cpt)
	    {
		g_anemo_max_day_cpt = g_anemo_cpt;
		g_anemo_cpt = 0;
		sprintf(g_anemo_max_day_string,"%d.%d km/h le %02d/%02d à %02dh%02d",
		    (uint16_t)(g_anemo_max_day_cpt * ANEMO_UNIT_SEC),
		    (uint16_t)((g_anemo_max_day_cpt * ANEMO_10_SEC)%10), g_day, g_mon, g_hour, g_min);
	    }

	    g_anemo_cpt = 0;

#ifdef DEBUG_METEO
	    Serial.print("Meteo cpt millis = ");Serial.println(g_cpt_milli);
	    sprintf(g_pluvio_string,"%d mm", (uint16_t)(g_pluvio_cpt * PLUVIO_UNIT));
	    Serial.print("g_pluvio_cpt = ");Serial.print(g_pluvio_cpt);Serial.print(" / ");Serial.println(g_pluvio_string);
	    Serial.print("g_pluvio_night_cpt = ");Serial.print(g_pluvio_night_cpt);Serial.print(" / ");
	    Serial.println(g_pluvio_night_string);
	    Serial.print("g_anemo_cpt  = ");Serial.print(g_anemo_cpt);Serial.print(" / ");Serial.println(g_anemo_string);
	    Serial.print("g_anemo_max_day_cpt = ");Serial.print(g_anemo_max_day_cpt);Serial.print(" / ");
	    Serial.println(g_anemo_max_day_string);
	    g_cpt_milli = 0;
#endif
	}
#ifdef DEBUG_METEO
	else
	{
	    g_cpt_milli++;
	}
#endif

	/* heure creuse ? */
	if (((g_hour100 > 200) && (g_hour100 < 700)) ||
	    ((g_hour100 > 1400) && (g_hour100 < 1700)))
	{
	    edf = &g_edf_hc;
	}
	else
	{
	    edf = &g_edf_hp;
	}

	if ((millis() - g_beginWait3msec) >= 3)
	{
	    g_beginWait3msec = millis();
	    edf->curr = analogRead(PIN_EDF);

#ifdef DEBUG_EDF
	    uint16_t value;
	    value = edf->curr;
	    Serial.print(value);
#endif

	    if (edf->curr > THRESHOLD_EDF)
	    {
		edf->state_curr = 1;
#ifdef DEBUG_EDF
		Serial.println("  1");
#endif
	    }
	    else
	    {
		edf->state_curr = 0;
#ifdef DEBUG_EDF
		Serial.println("  0");
#endif
	    }

	    if (edf->state_curr != edf->state_old)
	    {
		if (edf->state_curr == 0)
		{
		    digitalWrite(PIN_OUT_EDF, LIGHT_OFF);
		}
		else
		{
		    edf->value++;
		    digitalWrite(PIN_OUT_EDF, LIGHT_ON);
		}

		edf->state_old = edf->state_curr;
	    }
	}
    }
}


void process_time(void)
{
    if (g_process_time != PROCESS_OFF)
    {
	if (g_sec != second())
	{
	    g_hour = hour();
	    g_min  = minute();
	    g_hour100 = (100*g_hour + g_min);
	    g_sec  = second();
	    g_day  = day();
	    g_mon  = month();
	    g_year = year();

	    /* save current date and clock in global string var */
	    digitalClock();
	    digitalDate();
	}
    }
}

void callback_wait_pdomotix(void)
{
    /* restart process */
    g_process_domotix = PROCESS_ON;
}

void callback_buzz_portecellier_on(void)
{
    /* Active Buzzer */
    analogWrite(PIN_OUT_BUZZER, 220);
    g_evt_buzz_before5min_off.timeout = 1000;
    event_add(&g_evt_buzz_before5min_off, callback_buzz_portecellier_off);
}

void callback_buzz_portecellier_off(void)
{
    /* Stop buzzer */
    analogWrite(PIN_OUT_BUZZER, 0);
    g_evt_buzz_before5min_on.timeout = 500;
    event_add(&g_evt_buzz_before5min_on, callback_buzz_portecellier_on);
}

void callback_wait_portecellier(void)
{
    /* Send alerte */
    event_del(&g_evt_buzz_before5min_off);
    event_del(&g_evt_buzz_before5min_on);
    if (g_temperature_ext > 23)
    {
	send_SMS("Attention, il fait chaud et la porte du cellier est ouverte");
    }
    else
    {
	send_SMS_alerte("La porte exterieur du cellier est ouverte depuis 5min");
    }
    analogWrite(PIN_OUT_BUZZER, 220);
}

void event_del(event_t *event)
{
    if ((event->id >= 0) && (event->id < NB_DELAY_MAX))
    {
	g_delay[event->id].delay_inuse = 0;
#ifdef DEBUG_EVT
	Serial.print("event_del = ");
	Serial.println(event->id);
#endif
    }
}

void event_add(event_t *event, callback_delay call_after_delay)
{
    int8_t index;

    for(index = 0; index < NB_DELAY_MAX; index++)
    {
	if (g_delay[index].delay_inuse == 0)
	{
	    g_delay[index].delay_start  = millis();
	    g_delay[index].cb		= call_after_delay;
	    g_delay[index].delay_inuse  = 1;
	    g_delay[index].delay_wait   = event->timeout;
	    event->id = index;

#ifdef DEBUG_EVT
	    Serial.print("event_add = ");
	    Serial.println(index);
#endif
	    return;
	}
    }

    /* If there's no more buffer available, then wait and call CB
     */
#ifdef DEBUG_EVT
    Serial.println("/!\ warning No more buffer available");
    Serial.println(index);
#endif

    delay(event->timeout);
    if (call_after_delay != NULL)
    {
	call_after_delay();
    }
    return;
}


void process_delay(void)
{
    uint8_t index;

    if (g_process_delay != PROCESS_OFF)
    {
	for(index = 0; index < NB_DELAY_MAX; index++)
	{
	    if (g_delay[index].delay_inuse)
	    {
		if (millis() - g_delay[index].delay_start >= g_delay[index].delay_wait)
		{
		    /* call CB
		     */
		    if (g_delay[index].cb != NULL)
		    {
			g_delay[index].cb();
		    }
		    g_delay[index].delay_inuse = 0;
#ifdef DEBUG_EVT
		    Serial.print("free = ");
		    Serial.println(index);
#endif
		    return;
		}
	    }
	}
    }
}

void process_schedule(void)
{
    uint8_t  half_month;
    char     data1[15];
    char     data2[15];

    if (g_process_schedule != PROCESS_OFF)
    {
	/*************************************/
	/* Scheduling for temperature sensor */
	if ((g_hour100 == 800) || (g_hour100 == 1400) || (g_hour100 == 2000))
	{
	    if (g_sched_temperature == 0)
	    {
		g_sched_temperature = 1;

		/* write in file  */
		sprintf(data2,"%d C", g_temperature_ext);
		save_entry_string("M.txt", g_clock, data2);
	    }
	}
	else
	{
	    g_sched_temperature = 0;
	}

	/* Check negative temperature and send SMS to inform cars class */
	if (g_hour100 == 700)
	{
	    if (g_sched_daymin_sms == 0)
	    {
		g_sched_daymin_sms = 1;
		if (g_temperature_daymin <= 0)
		{
		    send_SMS("Givre probable sur les voitures");
		}
	    }
	}
	else
	{
	    g_sched_daymin_sms = 0;
	}

	/* reset pluvio night at 20h00 */
	if ((g_hour100 == 2000) || (g_hour100 == 800))
	{
	    if (g_sched_night == 0)
	    {
		if (g_hour100 == 800)
		{
		    sprintf(g_pluvio_night_string,"%d mm", (uint16_t)(g_pluvio_night_cpt * PLUVIO_UNIT));
		}
		else
		{
		    /* g_hour100 == 2000 */
		    g_pluvio_night_cpt = 0;
		}

		g_sched_night = 1;
	    }
	}
	else
	{
	    g_sched_night = 0;
	}

	/* reset temperature and meteo values of the day at midnight */
	if (g_hour100 == 0)
	{
	    if (g_sched_midnight == 0)
	    {
		g_sched_midnight = 1;

		/* reset meteo values */
		if (g_pluvio_cpt >= g_pluvio_max_cpt)
		{
		    g_pluvio_max_cpt = g_pluvio_cpt;
		    sprintf(g_pluvio_max_string,"%d mm le %02d/%02d", (uint16_t)(g_pluvio_max_cpt * PLUVIO_UNIT), g_day, g_mon);
		    g_pluvio_max_cpt_day = g_day;
		    g_pluvio_max_cpt_mon = g_mon;
		}
		g_pluvio_cpt = 0;

		if (g_anemo_max_day_cpt >= g_anemo_max_year_cpt)
		{
		    g_anemo_max_year_cpt = g_anemo_max_day_cpt;
		    sprintf(g_anemo_max_year_string,"%d.%d km/h le %02d/%02d à %02dh%02d",
			(uint16_t)(g_anemo_max_year_cpt * ANEMO_UNIT_SEC),
			(uint16_t)((g_anemo_max_year_cpt * ANEMO_10_SEC)%10), g_day, g_mon, g_hour, g_min);
		    g_anemo_max_year_cpt_hour = g_hour;
		    g_anemo_max_year_cpt_min = g_min;
		    g_anemo_max_year_cpt_day = g_day;
		    g_anemo_max_year_cpt_mon = g_mon;
		}
		g_anemo_max_day_cpt = 0;

		/* reset temperatures counters*/
		g_temperature_daymin = g_temperature_ext;
		g_temperature_daymax = g_temperature_ext;

		/* write in file  edf counters */
		sprintf(data1,"%lu", g_edf_hc.value);
		save_entry_string("hc.txt", g_clock, data1);
		sprintf(data1,"%lu", g_edf_hp.value);
		save_entry_string("hp.txt", g_clock, data1);

		/* write to files edf week counters */
		if (g_week == 1)
		{
		    sprintf(data1,"%lu", (uint32_t)(g_edf_hc.value/1000));
		    sprintf(data2,"%lu", (uint32_t)((g_edf_hc.value - g_edf_week_hc)/1000));
		    save_entry_string("U.txt", data1, data2);
		    g_edf_week_hc = g_edf_hc.value;

		    sprintf(data1,"%lu", (uint32_t)(g_edf_hp.value/1000));
		    sprintf(data2,"%lu", (uint32_t)((g_edf_hp.value - g_edf_week_hp)/1000));
		    save_entry_string("V.txt", data1, data2);
		    g_edf_week_hp = g_edf_hp.value;
		}

		/* change day of the week */
		if (g_week == 7)
		    g_week = 1;
		else
		    g_week++;

		/* save all in eeprom */
		save_eeprom();
	    }
	}
	else
	{
	    g_sched_midnight = 0;
	}

	/* reset temperature of the year at first january */
	if ((g_hour100 == 0) && (g_day == 1) && (g_mon == 1))
	{
	    if (g_sched_temp_year == 0)
	    {
		g_sched_temp_year = 1;

		g_temperature_yearmin = 60;
		g_temperature_yearmax = -60;
	    }
	}
	else
	{
	    g_sched_temp_year = 0;
	}

	/*************************************/
	/* check if door must be opened or closed */
	if (g_day < 16)
	    half_month = 0;
	else
	    half_month = 1;

	if (g_hour100 == g_time_to_open[2*g_mon + half_month])
	{
	    if (g_sched_door_already_opened == 0)
	    {
		g_sched_door_already_opened = 1;

		/* Open the door */
		digitalWrite(PIN_OUT_POULAILLER_ACTION, 1);
		delay(1000);
		digitalWrite(PIN_OUT_POULAILLER_ACTION, 0);
	    }
	}
	else
	{
	    g_sched_door_already_opened = 0;
	}

	if (g_hour100 == g_time_to_close[2*g_mon + half_month])
	{
	    if (g_sched_door_already_closed == 0)
	    {
		g_sched_door_already_closed = 1;

		/* close the door */
		digitalWrite(PIN_OUT_POULAILLER_ACTION, 1);
		delay(1000);
		digitalWrite(PIN_OUT_POULAILLER_ACTION, 0);
	    }
	}
	else
	{
	    g_sched_door_already_closed = 0;
	}

	if (g_start == 0)
	{
	    sprintf(g_start_date,"%s %s", g_date, g_clock);
	    g_start = 1;
	}
    }
}

void process_action(void)
{
    uint8_t action_done;
    char data[15];

    if (g_process_action != PROCESS_ACTION_NONE)
    {
	action_done = 1;
	switch(g_process_action)
	{
	    case PROCESS_ACTION_LIGHT_1:
	    {
		g_lampe1 = g_recv_gsm[0];
		digitalWrite(PIN_OUT_LAMPE_1, g_lampe1);
	    }break;
	    case PROCESS_ACTION_CRITICAL_TIME:
	    {
		if (g_recv_gsm[0] == 0)
		{
		    g_critical_alertes = 0;
		}
		else
		{
		    g_critical_alertes = 1;
		}
	    }
	    break;
	    case PROCESS_ACTION_LAMPE:
	    {
		digitalWrite(PIN_OUT_LAMPE_1, g_lampe1);
		digitalWrite(PIN_OUT_LAMPE_2, g_lampe2);
		digitalWrite(PIN_OUT_LAMPE_3, g_lampe3);
		digitalWrite(PIN_OUT_LAMPE_4, g_lampe4);
	    }break;
	    case PROCESS_ACTION_EDF:
	    {
		/* write in file  */
		sprintf(data,"%lu", g_edf_hc.value);
		save_entry_string("hc.txt", g_clock, data);

		sprintf(data,"%lu", g_edf_hp.value);
		save_entry_string("hp.txt", g_clock, data);
	    }
	    break;
	    case PROCESS_ACTION_TIMEZONE:
	    {
		if ((g_timezone != 1) && (g_timezone != 2))
		{
		    g_timezone = 2;
		}

		/* save all in eeprom */
		save_eeprom();
	    }
	    break;
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


void process_do_it(void)
{
    uint8_t is_gsm_ready;

    /* do it every time in order to be sure to be allowed to send msg to GSM */
    is_gsm_ready = digitalRead(PIN_GSM);
    if (is_gsm_ready != g_init_gsm)
    {
	g_init_gsm = is_gsm_ready;
	digitalWrite(PIN_OUT_GSM_INIT, g_init_gsm);
    }
}


void loop(void)
{
    process_time();
    process_ethernet();
#ifdef DEBUG_SERIAL
    process_serial();
#endif
    process_recv_gsm();
    process_domotix();
    process_domotix_quick();
    process_schedule();
    process_action();
    process_delay();
    process_do_it();
}
