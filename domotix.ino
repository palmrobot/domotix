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
#define PIN_A1		A1
#define PIN_A2		A2
#define PIN_A3		A3
#define PIN_A4		A4
#define PIN_A5		A5

#define PIN_9		9
#define PIN_8		8
#define PIN_7		7
#define PIN_6		6
#define PIN_5		5
#define CS_PIN_SDCARD   4
#define PIN_3		3
#define PIN_2		2



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
#define LINE_START_BUFF			1
char g_line[LINE_MAX_LEN+1];

#define BUFF_MAX_SIZE			200
uint8_t g_buff[BUFF_MAX_SIZE];

char g_root_filename[] = "index.htm";

typedef struct code_t
{
    const char *name0;
    uint8_t len0;
    const char *name1;
    uint8_t len1;
};

#define WEB_CODE_1		'1'
#define WEB_CODE_2		'2'
#define WEB_CODE_3		'3'

code_t g_code[] = { {"Fermee", 6, "Ouverte", 7},  /* code 1 */
		    {"Eteinte", 7, "Allumee", 7}, /* code 2 */
		    {"ok", 2, "ko", 2} }; /* code 3 */


EthernetServer g_server(9090);
EthernetClient g_client;


#define WEB_GET			1
#define WEB_EOL			2
#define WEB_END			4
#define WEB_GET_ROOT		8
#define WEB_ERROR		16

uint8_t g_page_web   = 0;
uint16_t g_req_count = 0;
File g_file_html;
time_t g_prevDisplay = 0;
char g_bufferTxt[80];

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


/********************************************************/
/*      CONSTANT STRING		                        */
/********************************************************/
const char g_txtError404[] PROGMEM = "HTTP/1.1 404 Not Found";



/********************************************************/
/*      Init			                        */
/********************************************************/
void setup(void)
{
    /* init Process */
    g_process_serial   = PROCESS_SERIAL_ON;
    g_process_ethernet = PROCESS_ETHERNET_ON;
    g_process_domotix  = PROCESS_DOMOTIX_OFF;
    g_process_time     = PROCESS_TIME_OFF;

    /* initialize serial communications at 115200 bps */
    Serial.begin(115200);

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

    /* Init Port In/Out */
    /*pinMode(PIN_LED, OUTPUT);*/

    /* Init global var */


#ifdef DEBUG
    Serial.print("Init OK\r\n");
#endif
}


/********************************************************/
/*      Web                                             */
/********************************************************/

void deal_with_code(char type, uint8_t code)
{
    uint8_t current_state;
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
	default:
	{
	    ptr_code = &g_code[0];
	}break;
    }

    switch (type)
    {
	case 'A':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'B':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'C':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'D':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'E':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'F':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'G':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'H':
	{
	    current_state = 1;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'I':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'J':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'K':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'L':
	{
	    current_state = 1;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'M':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'N':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'O':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'P':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'Q':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'R':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'S':
	{
	    current_state = 1;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'T':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'U':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'V':
	{
	    current_state = 0;
	    if (current_state == 1)
		g_client.write((const uint8_t*)ptr_code->name1, ptr_code->len1);
	    else
		g_client.write((const uint8_t*)ptr_code->name0, ptr_code->len0);
	}break;
	case 'W':
	{
	    digitalClockDisplay();
	}
    }
}

void send_file_to_client(File *file)
{
    uint8_t index;
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

    file->close();
}


void send_resp_to_client(File *file)
{
    uint16_t index;

    index = 0;
    while (file->available())
    {
	g_buff[index] = file->read();
	index++;
	if (index >= BUFF_MAX_SIZE)
	{
	    g_client.write(g_buff, index);
	    index = 0;
	}
    }
    if (index > 0)
	g_client.write(g_buff, index);

    file->close();
}

/********************************************************/
/*      NTP functions                                   */
/********************************************************/
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
    g_client.println();
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
    Serial.println("No NTP :-(");
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

void printwebln(char *text)
{
    strcpy_P(buffer,text);
   strcpy_P(buffer, (PGM_P)pgm_read_word(&(dayNames_P[day])));

    if (ln == 1)
    {
	g_client.println(buffer);
    }
    else
    {
	g_client.print(buffer);
    }
}

void printserial(char *text, uint8_t ln)
{
    char buffer[80];
    strcpy_P(buffer,text);

    if (ln == 1)
    {
	g_client.println(buffer);
    }
    else
    {
	g_client.print(buffer);
    }
}


/********************************************************/
/*      Process                                         */
/********************************************************/

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

void process_ethernet(void)
{
    uint8_t count;
    char *filename;
    char *end_filename;

    if (g_process_ethernet != PROCESS_ETHERNET_OFF)
    {
	g_client = g_server.available();
	if (g_client)
	{
	    g_page_web  = 0;
	    g_req_count = 0;

	    while (g_client.connected())
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
			/* Got a complete line */
			/* Set '\0' to the end of buffer for string treatment */
			g_line[g_req_count] = '\0';

			/* EOL */
			g_page_web |= WEB_EOL;
#ifdef DEBUG
			Serial.print("End of Line ");
#endif
		    }

		    g_req_count++;

		    if (g_req_count >= LINE_MAX_LEN)
		    {
			/* at the end of buffer until end of line */
			g_req_count--;
		    }

		    /********** Parsing Line received **************/
		    if ((g_page_web & WEB_EOL) == WEB_EOL)
		    {
		        if (strstr(g_line, "GET /") != 0)
			{
			    if (g_line[5] == ' ')
			    {
				filename = g_root_filename;
				g_page_web |= WEB_GET_ROOT;
			    }
			    else
			    {
				filename = g_line + 5;
			    }

			    end_filename = strstr(g_line," HTTP");
			    if (end_filename != NULL)
				end_filename[0] = '\0';
#ifdef DEBUG
			    Serial.print("found file:");Serial.println(filename);
#endif

			    /* Try open file to send*/
			    g_file_html = SD.open(filename);
			    if (g_file_html)
			    {
				/* send 200 OK */
				g_client.println("HTTP/1.1 200 OK");
				if (strstr(filename, ".htm") != 0)
				{
				    g_client.println("Content-Type: text/html");

				    /* end of header */
				    g_client.println();

				    send_file_to_client(&g_file_html);

				    break;
				}
				else if (strstr(filename, ".css") != 0)
				{
				    g_client.println("Content-Type: text/css");
				}
				else if (strstr(filename, ".jpg") != 0)
				{
				    g_client.println("Content-Type: image/jpeg");
				    /* g_client.println("Cache-Control: max-age=2592000"); */
				}
				else if (strstr(filename, ".ico") != 0)
				{
				    g_client.println("Content-Type: image/x-icon");
				    /* g_client.println("Cache-Control: max-age=2592000"); */
				}
				else
				    g_client.println("Content-Type: text");

				/* end of header */
				g_client.println();

				send_resp_to_client(&g_file_html);

				break;
			    }
			    else
			    {
				g_client.println("HTTP/1.1 404 Not Found");
				/* g_client.println("Content-Type: text/html"); */
				g_client.println();
				/* g_client.println("<h2>Domotix Error: File Not Found!</h2>"); */

				break;
			    }
			}
			else
			{
			    g_client.println("HTTP/1.1 404 Not Found");
			    /* g_client.println("Content-Type: text/html"); */
			    g_client.println();
			    /* g_client.println("<h2>Domotix Error: GET /  Not Found!</h2>"); */

			    break;
			}
		    }
		}
	    }
	    /* close connection */
	    delay(1);
	    g_client.stop();
	}
    }
}


void process_domotix(void)
{
    if (g_process_domotix != PROCESS_DOMOTIX_OFF)
    {



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
    process_serial();
    process_time();
    process_domotix();
}
