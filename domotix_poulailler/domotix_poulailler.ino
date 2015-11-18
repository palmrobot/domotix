 #include <Time.h>


/********************************************************/
/*      Pins   definitions                              */
/********************************************************/

#define PIN_IN_SENSOR_OPENED		2
#define PIN_IN_SENSOR_CLOSED		3

#define PIN_OUT_MOT_DIR_2		5
#define PIN_OUT_MOT_DIR_1		6
#define PIN_OUT_MOT_SPEED		7

#define PIN_IN_SENSOR_ACTION_MOVE	A1
#define PIN_IN_SENSOR_PONDOIR_GAUCHE	A2  /* Grey */
#define PIN_IN_SENSOR_PONDOIR_DROITE	A3  /* Brown */

/* domotix I/O */
#define PIN_IN_DOMOTIX_ACTION_MOVE	11 /* Blue */
#define PIN_OUT_DOMOTIX_STATE_DOOR	12 /* Yellow */
#define PIN_OUT_DOMOTIX_POULE_DROITE	8  /* Brown */
#define PIN_OUT_DOMOTIX_POULE_GAUCHE	9  /* Grey */

#define STATE_OPENNING			0
#define STATE_OPENED			1
#define STATE_CLOSING			2
#define STATE_CLOSED			3
#define STATE_UNKNOWN			4

#define TIME_TO_CLOSE_THE_DOOR		1800
#define TIME_TO_OPEN_THE_DOOR		700
#define TIME_TO_RESET_COUNTERS		0

#define THRESHOLD_POULE			400

#define DOOR_CLOSED			1
#define DOOR_OPENED			0

#define PONDOIR_POULE			0
#define PONDOIR_EMPTY			1

uint8_t g_process_serial;
uint8_t g_process_action;

uint8_t g_process_delay;
#define PROCESS_DELAY_OFF			0
#define PROCESS_DELAY_ON			1

uint8_t g_state_current;

typedef void (*callback_delay) (void);

typedef struct
{
    uint32_t delay_start;
    uint32_t delay_wait;
    uint8_t  delay_inuse;
    callback_delay cb;
}delay_t;

#define NB_DELAY_MAX			10
delay_t g_delay[NB_DELAY_MAX];

void open_the_door(void)
{
    /* open the door */
    digitalWrite(PIN_OUT_MOT_DIR_1, LOW);
    digitalWrite(PIN_OUT_MOT_DIR_2, HIGH);
    digitalWrite(PIN_OUT_MOT_SPEED, HIGH);
    g_state_current = STATE_OPENNING;
}

void close_the_door(void)
{
    /* open the door */
    digitalWrite(PIN_OUT_MOT_DIR_1, HIGH);
    digitalWrite(PIN_OUT_MOT_DIR_2, LOW);
    digitalWrite(PIN_OUT_MOT_SPEED, HIGH);
    g_state_current = STATE_CLOSING;
}

void setup()
{
    uint8_t opened;
    uint8_t closed;

    /* Init input/output */
    pinMode(PIN_OUT_MOT_DIR_1, OUTPUT);
    pinMode(PIN_OUT_MOT_DIR_2, OUTPUT);
    pinMode(PIN_OUT_MOT_SPEED, OUTPUT);
    pinMode(13, OUTPUT);
    digitalWrite(13, LOW);

    pinMode(PIN_IN_SENSOR_OPENED, INPUT);
    pinMode(PIN_IN_SENSOR_CLOSED, INPUT);
    pinMode(PIN_IN_SENSOR_ACTION_MOVE, INPUT);

    /* For Domotix */
    pinMode(PIN_OUT_DOMOTIX_STATE_DOOR, OUTPUT);
    pinMode(PIN_OUT_DOMOTIX_POULE_GAUCHE, OUTPUT);
    pinMode(PIN_OUT_DOMOTIX_POULE_DROITE, OUTPUT);
    pinMode(PIN_IN_DOMOTIX_ACTION_MOVE, INPUT);

    digitalWrite(PIN_OUT_DOMOTIX_STATE_DOOR, DOOR_CLOSED);
    digitalWrite(PIN_OUT_DOMOTIX_POULE_GAUCHE, PONDOIR_EMPTY);
    digitalWrite(PIN_OUT_DOMOTIX_POULE_DROITE, PONDOIR_EMPTY);

    /* Init global variables */
    g_process_serial	= 0;
    g_process_action    = 1;

    opened  = digitalRead(PIN_IN_SENSOR_OPENED);
    closed = digitalRead(PIN_IN_SENSOR_CLOSED);
    if (opened == 1)
    {
	g_state_current  = STATE_OPENED;
    }
    else if (closed == 1)
    {
	g_state_current  = STATE_CLOSED;
    }
    else
    {
	g_state_current  = STATE_OPENNING;
    }

    /* initialize serial communications at 115200 bps: */
    Serial.begin(115200);
    delay(100);
}


void process_action(void)
{
    uint8_t move_the_door;
    uint8_t opened;
    uint8_t closed;
    uint8_t poule_gauche;
    uint8_t poule_droite;

    if (g_process_action != 0)
    {
	opened  = digitalRead(PIN_IN_SENSOR_OPENED);
	closed = digitalRead(PIN_IN_SENSOR_CLOSED);

	if (closed == 1)
	{
	    if (g_state_current == STATE_CLOSING)
	    {
		g_state_current  = STATE_CLOSED;

		/* Stop immediatly */
		digitalWrite(PIN_OUT_MOT_DIR_1, LOW);
		digitalWrite(PIN_OUT_MOT_DIR_2, LOW);
		digitalWrite(PIN_OUT_MOT_SPEED, HIGH);
		delay(1000);
		digitalWrite(PIN_OUT_MOT_SPEED, LOW);

		digitalWrite(PIN_OUT_DOMOTIX_STATE_DOOR, DOOR_CLOSED);
	    }
	}
	else if (opened == 1)
	{
	    if (g_state_current == STATE_OPENNING)
	    {
		g_state_current  = STATE_OPENED;

		/* Stop immediatly */
		digitalWrite(PIN_OUT_MOT_DIR_1, LOW);
		digitalWrite(PIN_OUT_MOT_DIR_2, LOW);
		digitalWrite(PIN_OUT_MOT_SPEED, HIGH);
		delay(1000);
		digitalWrite(PIN_OUT_MOT_SPEED, LOW);

		digitalWrite(PIN_OUT_DOMOTIX_STATE_DOOR, DOOR_OPENED);
	    }
	}

	/* Check if outdoor button is pressed */
	move_the_door  = digitalRead(PIN_IN_SENSOR_ACTION_MOVE);
	if (move_the_door == 0)
	{
	    /* Check if door must be openned or closed by action domotix */
	    move_the_door = digitalRead(PIN_IN_DOMOTIX_ACTION_MOVE);
	}

	if ((move_the_door) && ((g_state_current == STATE_CLOSED) || (g_state_current == STATE_CLOSING)))
	{
	    open_the_door();

	    /* wait some time before reading sensor */
	    delay(2000);
	}
	else if ((move_the_door) && ((g_state_current == STATE_OPENED) || (g_state_current == STATE_OPENNING)))
	{
	    close_the_door();

	    /* wait some time before reading sensor */
	    delay(2000);
	}

	poule_gauche = analogRead(PIN_IN_SENSOR_PONDOIR_GAUCHE);
	if (poule_gauche > THRESHOLD_POULE)
	{
	    digitalWrite(PIN_OUT_DOMOTIX_POULE_GAUCHE, PONDOIR_POULE);
	}
	else
	{
	    digitalWrite(PIN_OUT_DOMOTIX_POULE_GAUCHE, PONDOIR_EMPTY);
	}

	poule_droite = analogRead(PIN_IN_SENSOR_PONDOIR_DROITE);
	if (poule_droite > THRESHOLD_POULE)
	{
	    digitalWrite(PIN_OUT_DOMOTIX_POULE_DROITE, PONDOIR_POULE);
	}
	else
	{
	    digitalWrite(PIN_OUT_DOMOTIX_POULE_DROITE, PONDOIR_EMPTY);
	}
    }
}


void wait_some_time( unsigned long time_to_wait, callback_delay call_after_delay);

void wait_some_time( unsigned long time_to_wait, callback_delay call_after_delay)
{
    uint32_t current_millis;
    uint8_t index;

    for(index = 0; index < NB_DELAY_MAX; index++)
    {
	if (g_delay[index].delay_inuse == 0)
	{
	    current_millis = millis();
	    g_delay[index].delay_start  = current_millis;
	    g_delay[index].cb		= call_after_delay;
	    g_delay[index].delay_inuse  = 1;
	    return;
	}
    }

    /* no more delay in tab availble
     */
    delay(time_to_wait);
}

void process_delay(void)
{
    uint32_t current_millis;
    uint8_t index;

    if (g_process_delay != PROCESS_DELAY_OFF)
    {
	for(index = 0; index < NB_DELAY_MAX; index++)
	{
	    if (g_delay[index].delay_inuse)
	    {
		current_millis = millis();
		if ((current_millis - g_delay[index].delay_start) > g_delay[index].delay_wait)
		{
		    /* call CB
		     */
		    if (g_delay[index].cb != NULL)
		    {
			g_delay[index].cb();
		    }
		    g_delay[index].delay_inuse = 0;
		    return;
		}
	    }
	}
    }
}

void process_serial(void)
{
    char value_read;

    if (g_process_serial)
    {
	/* if we get a valid char, read char */
	if (Serial.available() > 0)
	{
	    /* get incoming byte: */
	    value_read = Serial.read();
	    Serial.println(value_read);
	    switch (value_read)
	    {
		case 'a':
		{
		    digitalWrite(PIN_OUT_MOT_DIR_1, LOW);
		    digitalWrite(PIN_OUT_MOT_DIR_2, HIGH);
		    digitalWrite(PIN_OUT_MOT_SPEED, HIGH);
		}break;
		case 'b':
		{
		    digitalWrite(PIN_OUT_MOT_DIR_1, HIGH);
		    digitalWrite(PIN_OUT_MOT_DIR_2, LOW);
		    digitalWrite(PIN_OUT_MOT_SPEED, HIGH);
		}break;
		case 'c':
		{
		    digitalWrite(PIN_OUT_MOT_DIR_1, LOW);
		    digitalWrite(PIN_OUT_MOT_DIR_2, LOW);
		    digitalWrite(PIN_OUT_MOT_SPEED, HIGH);
		}break;
		case 'd':
		{
		    digitalWrite(PIN_OUT_MOT_DIR_1, HIGH);
		    digitalWrite(PIN_OUT_MOT_DIR_2, HIGH);
		    digitalWrite(PIN_OUT_MOT_SPEED, HIGH);
		}break;
		case 'e':
		{
		    digitalWrite(PIN_OUT_MOT_DIR_1, LOW);
		    digitalWrite(PIN_OUT_MOT_DIR_2, LOW);
		    digitalWrite(PIN_OUT_MOT_SPEED, LOW);
		}break;

		default:

		break;
	    }
	}
    }
}

void loop()
{
    process_serial();
    process_action();
}




