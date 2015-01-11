#include <avr/pgmspace.h>
#include <SPI.h>
#include <Time.h>
#include <SD.h>
#include <Ethernet.h>
#include <LiquidCrystal.h>

/* #define DEBUG */

/********************************************************/
/*      Pin  definitions                                */
/********************************************************/
#define PIN_A0		A0
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
#define CS_PIN_SDCARD			4
#define PIN_LINGERIE_CUISINE		3 /* K */
#define PIN_GARAGE_FOND			2 /* H */
#define PIN_CUISINE_EXT			1 /* L */
#define PIN_LINGERIE_FENETRE		0 /* N */


/********************************************************/
/*      Process definitions                             */
/********************************************************/
uint8_t g_process_serial;
#define PROCESS_SERIAL_OFF		0
#define PROCESS_SERIAL_ON		1

uint8_t g_process_ethernet;
#define PROCESS_ETHERNET_OFF		0
#define PROCESS_ETHERNET_ON		1


uint8_t g_process_domotix;
#define PROCESS_DOMOTIX_OFF		0
#define PROCESS_DOMOTIX_ON		1

uint8_t g_process_time;
#define PROCESS_TIME_OFF		0
#define PROCESS_TIME_ON			1

/********************************************************/
/*      Global definitions                              */
/********************************************************/
uint8_t g_mac_addr[] = { 0x90, 0xA2, 0xDA, 0x00, 0xFF, 0x86};
uint8_t g_ip_addr[] = { 192, 168, 5, 20 };

#define LINE_MAX_LEN			64
char g_line[LINE_MAX_LEN + 1];

#define BUFF_MAX_SIZE			100
uint8_t g_buff[BUFF_MAX_SIZE];

typedef struct code_t
{
    const char *name[2];
};

#define WEB_CODE_1		'1'
#define WEB_CODE_2		'2'
#define WEB_CODE_3		'3'
#define WEB_CODE_4		'4'

code_t g_code[] = { {"Ouverte", "Fermee"},  /* code 1 */
		    {"Allumee", "Eteinte"}, /* code 2 */
		    {"ko", "ok"}, /* code 3 */
		    {"Ouvert", "Ferme"} }; /* code 4 */


EthernetServer g_server(9090);
EthernetClient g_client;

typedef struct state_uint8_t
{
    uint8_t old;
    uint8_t curr;
};

typedef struct state_int_t
{
    int old;
    int curr;
};

#define FILE_MAX_LEN			13
typedef struct file_web_t
{
    File  fd;
    char name[FILE_MAX_LEN + 1];
};

state_uint8_t g_garage_droite; /* A */
state_uint8_t g_garage_gauche; /* B */
state_uint8_t g_garage_fenetre; /* C */
state_int_t g_garage_lumiere_etabli; /* D */
state_uint8_t g_cellier_porte_ext; /* E */
state_uint8_t g_cellier_porte; /* F */
state_int_t g_cellier_lumiere; /* G */
state_uint8_t g_garage_porte; /* H */
state_int_t g_garage_lumiere; /* I */
state_int_t g_lingerie_lumiere; /* J */
state_uint8_t g_lingerie_porte_cuisine; /* K */
state_uint8_t g_cuisine_porte_ext; /* L */
state_int_t g_temperature_ext; /* M */
state_uint8_t g_lingerie_fenetre; /* N */

state_uint8_t g_default; /*  */

#define THRESHOLD_CMP_OLD	10
#define THRESHOLD_LIGHT_ON	70

#define WEB_GET			1
#define WEB_EOL			2
#define WEB_END			4
#define WEB_ERROR		16

uint8_t g_page_web   = 0;
uint16_t g_req_count = 0;
time_t g_prevDisplay = 0;
file_web_t g_filename;
uint8_t g_debug = 0;

/********************************************************/
/*      NTP			                        */
/********************************************************/

#define TIMEZONE			1
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


/********************************************************/
/*      CONSTANT STRING		                        */
/********************************************************/



/********************************************************/
/*      Init			                        */
/********************************************************/
void setup(void)
{
    /* Init Port In/Out */
    pinMode(PIN_GARAGE_DROITE, INPUT);
    pinMode(PIN_GARAGE_GAUCHE, INPUT);
    pinMode(PIN_GARAGE_FENETRE, INPUT);
    pinMode(PIN_CELLIER_PORTE_EXT, INPUT);
    pinMode(PIN_CELLIER_PORTE_INT, INPUT);
    pinMode(PIN_LINGERIE_CUISINE, INPUT);
    pinMode(PIN_GARAGE_FOND, INPUT);
    pinMode(PIN_CUISINE_EXT, INPUT);
    pinMode(PIN_LINGERIE_FENETRE, INPUT);

    /* init Process */
    g_process_serial   = PROCESS_SERIAL_ON;
    g_process_ethernet = PROCESS_ETHERNET_ON;
    g_process_domotix  = PROCESS_DOMOTIX_ON;
    g_process_time     = PROCESS_TIME_OFF;

#ifdef DEBUG
    /* initialize serial communications at 115200 bps */
    Serial.begin(115200);
#endif

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

    /* Init global var for web code */
    g_default.curr = 1;
    g_default.old  = 1;

    g_garage_droite.old = 0;
    g_garage_gauche.old = 0;
    g_garage_fenetre.old = 0;
    g_garage_lumiere_etabli.old = 0;
    g_cellier_porte_ext.old = 0;
    g_cellier_porte.old = 0;
    g_cellier_lumiere.old = 0;
    g_garage_porte.old = 0;
    g_garage_lumiere.old = 0;
    g_lingerie_lumiere.old = 0;
    g_lingerie_lumiere.old = 0;
    g_lingerie_porte_cuisine.old = 0;
    g_cuisine_porte_ext.old = 0;
    g_temperature_ext.old = 0;
    g_lingerie_fenetre.old = 0;

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


/********************************************************/
/*      Web                                             */
/********************************************************/

void deal_with_code(uint8_t type, uint8_t code)
{
    code_t *ptr_code;

    switch (code)
    {
	case WEB_CODE_1:
	{
	    ptr_code = &g_code[0];
	}break;
	case WEB_CODE_2:
	{
	    ptr_code = &g_code[1];
	}break;
	case WEB_CODE_3:
	{
	    ptr_code = &g_code[2];
	}break;
	case WEB_CODE_4:
	{
	    ptr_code = &g_code[3];
	}break;
	default:
	{
	    ptr_code = &g_code[0];
	}break;
    }

    switch (type)
    {
	case 'y':
	{
	    g_debug = 1;
	}break;
	case 'z':
	{
	    digitalClockDisplay();
	    /* debug code ( $y0 ) must be set after time in .html page */
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
		if (g_garage_lumiere_etabli.curr > THRESHOLD_LIGHT_ON)
		{
		    g_client.write((uint8_t*)ptr_code->name[0],
			strlen(ptr_code->name[0]));
		}
		else
		{
		    g_client.write((uint8_t*)ptr_code->name[1],
			strlen(ptr_code->name[1]));
		}
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
		if (g_cellier_lumiere.curr > THRESHOLD_LIGHT_ON)
		{
		    g_client.write((uint8_t*)ptr_code->name[0],
			strlen(ptr_code->name[0]));
		}
		else
		{
		    g_client.write((uint8_t*)ptr_code->name[1],
			strlen(ptr_code->name[1]));
		}
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
		if (g_garage_lumiere.curr > THRESHOLD_LIGHT_ON)
		{
		    g_client.write((uint8_t*)ptr_code->name[0],
			strlen(ptr_code->name[0]));
		}
		else
		{
		    g_client.write((uint8_t*)ptr_code->name[1],
			strlen(ptr_code->name[1]));
		}
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
		if (g_lingerie_lumiere.curr > THRESHOLD_LIGHT_ON)
		{
		    g_client.write((uint8_t*)ptr_code->name[0],
			strlen(ptr_code->name[0]));
		}
		else
		{
		    g_client.write((uint8_t*)ptr_code->name[1],
			strlen(ptr_code->name[1]));
		}
	    }
	}break;
	case 'K':
	{
	    g_client.write((uint8_t*)ptr_code->name[g_lingerie_porte_cuisine.curr],
		strlen(ptr_code->name[g_lingerie_porte_cuisine.curr]));
	}break;
	/* case 'L': */
	/* { */
	/*     g_client.write((uint8_t*)ptr_code->name[g_cuisine_porte_ext.curr], */
	/* 	strlen(ptr_code->name[g_cuisine_porte_ext.curr])); */
	/* }break; */
	case 'M':
	{
	    g_client.print(g_temperature_ext.curr);
	}break;
	/* case 'N': */
	/* { */
	/*     g_client.write((uint8_t*)ptr_code->name[g_lingerie_fenetre.curr], */
	/* 	strlen(ptr_code->name[g_lingerie_fenetre.curr])); */
	/* }break; */
	/* case 'O': */
	/* { */
	/* }break; */
	/* case 'P': */
	/* { */
	/* }break; */
	/* case 'Q': */
	/* { */
	/* }break; */
	/* case 'R': */
	/* { */
	/*     g_client.write((uint8_t*)ptr_code->name[g_default.curr], */
	/* 	strlen(ptr_code->name[g_default.curr])); */
	/* }break; */
	/* case 'S': */
	/* { */
	/*     g_client.write((uint8_t*)ptr_code->name[g_default.curr], */
	/* 	strlen(ptr_code->name[g_default.curr])); */
	/* }break; */
	/* case 'T': */
	/* { */
	/*     g_client.write((uint8_t*)ptr_code->name[g_default.curr], */
	/* 	strlen(ptr_code->name[g_default.curr])); */
	/* }break; */
	/* case 'U': */
	/* { */
	/*     g_client.write((uint8_t*)ptr_code->name[g_default.curr], */
	/* 	strlen(ptr_code->name[g_default.curr])); */
	/* }break; */
	/* case 'V': */
	/* { */
	/*     g_client.write((uint8_t*)ptr_code->name[1], */
	/* 	strlen(ptr_code->name[1])); */

	/* }break; */
	/* case 'W': */
	/* { */
	/*     g_client.write((uint8_t*)ptr_code->name[g_default.curr], */
	/* 	strlen(ptr_code->name[g_default.curr])); */
	/* }break; */
	/* case 'X': */
	/* { */
	/*     g_client.write((uint8_t*)ptr_code->name[g_default.curr], */
	/* 	strlen(ptr_code->name[g_default.curr])); */
	/* }break; */
	/* case 'Y': */
	/* { */
	/*     g_client.write((uint8_t*)ptr_code->name[g_default.curr], */
	/* 	strlen(ptr_code->name[g_default.curr])); */
	/* }break; */
	/* case 'Z': */
	/* { */
	/*     g_client.write((uint8_t*)ptr_code->name[g_default.curr], */
	/* 	strlen(ptr_code->name[g_default.curr])); */
	/* }break; */
	/* case 'a': */
	/* { */
	/*     g_client.write((uint8_t*)ptr_code->name[g_default.curr], */
	/* 	strlen(ptr_code->name[g_default.curr])); */
	/* }break; */
	/* case 'b': */
	/* { */
	/*     g_client.write((uint8_t*)ptr_code->name[g_default.curr], */
	/* 	strlen(ptr_code->name[g_default.curr])); */
	/* }break; */
	/* case 'c': */
	/* { */
	/*     g_client.write((uint8_t*)ptr_code->name[g_default.curr], */
	/* 	strlen(ptr_code->name[g_default.curr])); */
	/* }break; */
	default:
	
	break;
    }
}

void send_file_to_client(File *file)
{
    uint16_t index;
    uint8_t code;
    uint8_t type;

    index = 0;
    while (file->available())
    {
	g_buff[index] = file->read();

	 if (g_buff[index] == '$')
	 {
		/* first of all, send the last buffer without the '$'*/
		g_client.write(g_buff, index);
            	index = 0;

		/* then get the type */
		type = file->read();

		/* then get the code */
		code = file->read();

		deal_with_code(type, code);
	 }
	 else
	 {
	     index ++;
	     if (index >= BUFF_MAX_SIZE)
	     {
		 g_client.write(g_buff, index);
		 index = 0;
	     }
	 }
    }
    if (index > 0)
	g_client.write(g_buff, index);
}


void send_resp_to_client(File *fd)
{
    uint16_t index;

    index = 0;
    while (fd->available())
    {
	g_buff[index] = fd->read();
	index++;
	if (index >= BUFF_MAX_SIZE)
	{
	    g_client.write(g_buff, index);
	    index = 0;
	}
    }
    if (index > 0)
	g_client.write(g_buff, index);
}

/********************************************************/
/*      NTP functions                                   */
/********************************************************/

#ifdef DEBUG
void digitalClockDisplaySerial(void)
{
    Serial.print(hour());
    Serial.print (":");
    printDigitsSerial(minute());
    Serial.print (":");
    printDigitsSerial(second());
    Serial.print(" ");
    Serial.print(day());
    Serial.print("/");
    Serial.print(month());
    Serial.print("/");
    Serial.print(year());
    Serial.println();
}

void printDigitsSerial(int digits)
{
    if (digits < 10)
	Serial.print('0');
    Serial.print(digits);
}
#endif

void digitalClockDisplay(void)
{
    g_client.print(hour());
    g_client.print (":");
    printDigits(minute());
    g_client.print (":");
    printDigits(second());
    g_client.print(" ");
    g_client.print(day());
    g_client.print("/");
    g_client.print(month());
    g_client.print("/");
    g_client.print(year());
}

void printDigits(int digits)
{
    if (digits < 10)
	g_client.print('0');
    g_client.print(digits);
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
#ifdef DEBUG
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
#ifdef DEBUG
		    Serial.print(g_line[g_req_count]);
#endif

		    /* Search for end of line */
		    if ((g_line[g_req_count] == '\r') ||
			(g_line[g_req_count] == '\n'))
		    {
#ifdef DEBUG
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

#ifdef DEBUG
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
#ifdef DEBUG
	PgmPrintln("Connection Closed");
#endif
	}
    }
}

void process_domotix(void)
{
    if (g_process_domotix != PROCESS_DOMOTIX_OFF)
    {
	g_garage_droite.curr =  digitalRead(PIN_GARAGE_DROITE);
	if (g_garage_droite.curr != g_garage_droite.old)
	{
	    g_garage_droite.old = g_garage_droite.curr;
	    /* write in file
	     * Format :
	     *
	     */
#ifdef DEBUG
	    PgmPrint("Garage droite :");Serial.println(g_garage_droite.curr);
#endif
	}

	g_garage_gauche.curr =  digitalRead(PIN_GARAGE_GAUCHE);
	if (g_garage_gauche.curr != g_garage_gauche.old)
	{
	    g_garage_gauche.old = g_garage_gauche.curr;
	    /* write in file
	     * Format :
	     *
	     */
#ifdef DEBUG
	    PgmPrint("Garage gauche :");Serial.println(g_garage_gauche.curr);
#endif
	}

	g_garage_fenetre.curr =  digitalRead(PIN_GARAGE_FENETRE);
	if (g_garage_fenetre.curr != g_garage_fenetre.old)
	{
	    g_garage_fenetre.old = g_garage_fenetre.curr;
	    /* write in file
	     * Format :
	     *
	     */
#ifdef DEBUG
	    PgmPrint("Garage fenetre :");Serial.println(g_garage_fenetre.curr);
#endif
	}

	g_cellier_porte_ext.curr =  digitalRead(PIN_CELLIER_PORTE_EXT);
	if (g_cellier_porte_ext.curr != g_cellier_porte_ext.old)
	{
	    g_cellier_porte_ext.old = g_cellier_porte_ext.curr;
	    /* write in file
	     * Format :
	     *
	     */
#ifdef DEBUG
	    PgmPrint("Cellier porte ext :");Serial.println(g_cellier_porte_ext.curr);
#endif
	}

	g_cellier_porte.curr =  digitalRead(PIN_CELLIER_PORTE_INT);
	if (g_cellier_porte.curr != g_cellier_porte.old)
	{
	    g_cellier_porte.old = g_cellier_porte.curr;
	    /* write in file
	     * Format :
	     *
	     */
#ifdef DEBUG
	    PgmPrint("Cellier porte lingerie :");Serial.println(g_cellier_porte.curr);
#endif
	}

	g_lingerie_porte_cuisine.curr =  digitalRead(PIN_LINGERIE_CUISINE);
	if (g_lingerie_porte_cuisine.curr != g_lingerie_porte_cuisine.old)
	{
	    g_lingerie_porte_cuisine.old = g_lingerie_porte_cuisine.curr;
	    /* write in file
	     * Format :
	     *
	     */
#ifdef DEBUG
	    PgmPrint("Lingerie porte cuisine :");Serial.println(g_lingerie_porte_cuisine.curr);
#endif
	}

	g_garage_porte.curr =  digitalRead(PIN_GARAGE_FOND);
	if (g_garage_porte.curr != g_garage_porte.old)
	{
	    g_garage_porte.old = g_garage_porte.curr;
	    /* write in file
	     * Format :
	     *
	     */
#ifdef DEBUG
	    PgmPrint("Garage fond :");Serial.println(g_garage_porte.curr);
#endif
	}

	g_cuisine_porte_ext.curr = 1; /* digitalRead(PIN_CUISINE_EXT); */
	if (g_cuisine_porte_ext.curr != g_cuisine_porte_ext.old)
	{
	    g_cuisine_porte_ext.old = g_cuisine_porte_ext.curr;
	    /* write in file
	     * Format :
	     *
	     */
#ifdef DEBUG
	    PgmPrint("Cuisine porte ext:");Serial.println(g_cuisine_porte_ext.curr);
#endif
	}

	g_lingerie_fenetre.curr =  1; /* digitalRead(PIN_LINGERIE_FENETRE); */
	if (g_lingerie_fenetre.curr != g_lingerie_fenetre.old)
	{
	    g_lingerie_fenetre.old = g_lingerie_fenetre.curr;
	    /* write in file
	     * Format :
	     *
	     */
#ifdef DEBUG
	    PgmPrint("Lingerie fenetre:");Serial.println(g_lingerie_fenetre.curr);
#endif
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
	    /* write in file
	     * Format :
	     *
	     */
#ifdef DEBUG
	    PgmPrint("Garage lumiere etabli :");Serial.println(g_garage_lumiere_etabli.curr);
#endif
	}

	g_cellier_lumiere.curr =  analogRead(PIN_CELLIER_LUMIERE);
	if ((g_cellier_lumiere.curr > (g_cellier_lumiere.old + THRESHOLD_CMP_OLD)) ||
	    ((g_cellier_lumiere.curr + THRESHOLD_CMP_OLD) < g_cellier_lumiere.old))
	{
	    g_cellier_lumiere.old = g_cellier_lumiere.curr;
	    /* write in file
	     * Format :
	     *
	     */
#ifdef DEBUG
	    PgmPrint("Cellier lumiere :");Serial.println(g_cellier_lumiere.curr);
#endif

	}

	int value = analogRead(PIN_TEMP_EXT);
	g_temperature_ext.curr = (500.0 * value) / 1024;

	if ((g_temperature_ext.curr > (g_temperature_ext.old + 1)) ||
	    ((g_temperature_ext.curr + 1) < g_temperature_ext.old))
	{
	    g_temperature_ext.old = g_temperature_ext.curr;
	    /* write in file
	     * Format :
	     *
	     */
#ifdef DEBUG
	    PgmPrint("Temperature Ext:");Serial.println(g_temperature_ext.curr);
#endif
	}

	g_garage_lumiere.curr =  analogRead(PIN_GARAGE_LUMIERE);
	if ((g_garage_lumiere.curr > (g_garage_lumiere.old + THRESHOLD_CMP_OLD)) ||
	    ((g_garage_lumiere.curr + THRESHOLD_CMP_OLD) < g_garage_lumiere.old))
	{
	    g_garage_lumiere.old = g_garage_lumiere.curr;
	    /* write in file
	     * Format :
	     *
	     */
#ifdef DEBUG
	    PgmPrint("Garage lumiere :");Serial.println(g_garage_lumiere.curr);
#endif
	}

	g_lingerie_lumiere.curr =  analogRead(PIN_LINGERIE_LUMIERE);
	if ((g_lingerie_lumiere.curr > (g_lingerie_lumiere.old + THRESHOLD_CMP_OLD)) ||
	    ((g_lingerie_lumiere.curr + THRESHOLD_CMP_OLD) < g_lingerie_lumiere.old))
	{
	    g_lingerie_lumiere.old = g_lingerie_lumiere.curr;
	    /* write in file
	     * Format :
	     *
	     */
#ifdef DEBUG
	    PgmPrint("Garage lumiere :");Serial.println(g_lingerie_lumiere.curr);
#endif
	}

	delay(10);
    }
}


void process_time(void)
{
    time_t  curr_time;

    if (g_process_time != PROCESS_TIME_OFF)
    {
	if (timeStatus() != timeNotSet)
	{
	    curr_time = now();
	    if (curr_time != g_prevDisplay)
	    {
		g_prevDisplay = curr_time;
		digitalClockDisplay();
	    }
	}
    }
}



void loop(void)
{
    process_ethernet();
#ifdef DEBUG
    process_serial();
#endif
    process_time();
    process_domotix();
}
