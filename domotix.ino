#include <SD.h>
#include <SPI.h>
#include <Ethernet.h>

#define DEBUG

/********************************************************/
/*      Pin  definitions                                */
/********************************************************/
#define CS_PIN_SDCARD   4

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

/********************************************************/
/*      Init			                        */
/********************************************************/
void setup(void)
{
    /* initialize serial communications at 115200 bps */
    Serial.begin(115200);

    /* start the Ethernet connection and the server: */
    Ethernet.begin(g_mac_addr, g_ip_addr);
    g_server.begin();
    delay(100);

    /* Init SDCard */
    SD.begin(CS_PIN_SDCARD);
    delay(100);

    /* Init global var */

    /* init Process */
    g_process_serial   = PROCESS_SERIAL_ON;
    g_process_ethernet = PROCESS_ETHERNET_ON;
    g_process_domotix  = PROCESS_DOMOTIX_OFF;

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

#ifdef DEBUG
			    Serial.print("Bonjour\r\n");
#endif
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
				    g_client.println("Cache-Control: max-age=2592000");
				}
				else if (strstr(filename, ".ico") != 0)
				{
				    g_client.println("Content-Type: image/x-icon");
				    g_client.println("Cache-Control: max-age=2592000");
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
				g_client.println("Content-Type: text/html");
				g_client.println();
				g_client.println("<h2>Domotix Error: File Not Found!</h2>");

				break;
			    }
			}
			else
			{
			    g_client.println("HTTP/1.1 404 Not Found");
			    g_client.println("Content-Type: text/html");
			    g_client.println();
			    g_client.println("<h2>Domotix Error: GET /  Not Found!</h2>");

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

void loop(void)
{
    process_ethernet();
    process_serial();
    process_domotix();
}
