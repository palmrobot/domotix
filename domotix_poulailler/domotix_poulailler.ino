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

#define THRESHOLD_POULE			300

#define DOOR_CLOSED			1
#define DOOR_OPENED			0

#define PONDOIR_POULE			1
#define PONDOIR_EMPTY			0

uint8_t g_process_serial;
uint8_t g_process_door;
uint8_t g_process_poule;

uint8_t g_process_delay;
#define PROCESS_DELAY_OFF			0
#define PROCESS_DELAY_ON			1

#define BLOCKED_DOOR_TIMEOUT		3
#define CLOSING_DOOR_TIMEOUT		60
#define OPENING_DOOR_TIMEOUT		60

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

time_t g_current_time;
time_t g_openning_time;
time_t g_closing_time;

time_t g_current_poule_gauche_time;
time_t g_previous_poule_gauche_time;
uint8_t g_poule_gauche_state;
uint8_t g_poule_gauche_previous_state;

time_t g_current_poule_droite_time;
time_t g_previous_poule_droite_time;
uint8_t g_poule_droite_state;
uint8_t g_poule_droite_previous_state;

void open_the_door(void)
{
    /* open the door */
    digitalWrite(PIN_OUT_MOT_DIR_1, LOW);
    digitalWrite(PIN_OUT_MOT_DIR_2, HIGH);
    digitalWrite(PIN_OUT_MOT_SPEED, HIGH);
}

void close_the_door(void)
{
    /* open the door */
    digitalWrite(PIN_OUT_MOT_DIR_1, HIGH);
    digitalWrite(PIN_OUT_MOT_DIR_2, LOW);
    digitalWrite(PIN_OUT_MOT_SPEED, HIGH);
}

void stop_motor(void)
{
    delay(400);
    digitalWrite(PIN_OUT_MOT_DIR_1, LOW);
    digitalWrite(PIN_OUT_MOT_DIR_2, LOW);
    digitalWrite(PIN_OUT_MOT_SPEED, HIGH);
    delay(200);
    digitalWrite(PIN_OUT_MOT_SPEED, LOW);
}

void setup()
{
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
    g_process_door      = 1;
    g_process_poule     = 1;

    /* Set arbitrary state */
    g_state_current  = STATE_OPENNING;

    /* initialize serial communications at 115200 bps: */
    Serial.begin(115200);
    delay(100);

    setTime(0,0,0,20,11,15);
    g_current_time  = now();
    g_closing_time  = g_current_time;
    g_openning_time = g_current_time;
    g_previous_poule_gauche_time = g_current_time;
    g_previous_poule_droite_time = g_current_time;
    g_current_poule_gauche_time = g_current_time;
    g_current_poule_droite_time = g_current_time;

    g_poule_gauche_previous_state = 2;
    g_poule_droite_previous_state = 2;
    g_poule_gauche_state = 3;
    g_poule_droite_state = 3;
}


void process_door(void)
{
    uint8_t move_the_door;
    uint8_t opened;
    uint8_t closed;

    if (g_process_door != 0)
    {
	/* First, Check Current state */
	opened = digitalRead(PIN_IN_SENSOR_OPENED);
	closed = digitalRead(PIN_IN_SENSOR_CLOSED);

	/* Door is closed, check if it has just happened */
	if (closed == 1)
	{
	    if (g_state_current == STATE_CLOSING)
	    {
		g_state_current  = STATE_CLOSED;

		/* Stop immediatly */
		stop_motor();

		/* Then send info to Domotix */
		digitalWrite(PIN_OUT_DOMOTIX_STATE_DOOR, DOOR_CLOSED);
	    }
	    else if (g_state_current == STATE_OPENNING)
	    {
		/* check if door is blocked */
		g_current_time = now();
		if (g_current_time > (g_openning_time + BLOCKED_DOOR_TIMEOUT))
		{
		    /* something when wrong, door is blocked */
		    g_state_current  = STATE_CLOSED;

		    /* Stop immediatly */
		    stop_motor();

		    /* Then send info to Domotix */
		    digitalWrite(PIN_OUT_DOMOTIX_STATE_DOOR, DOOR_CLOSED);
		}
	    }
	}
	/* Door is openned, check if it has just happened */
	else if (opened == 1)
	{
	    if (g_state_current == STATE_OPENNING)
	    {
		g_state_current  = STATE_OPENED;

		/* Stop immediatly */
		stop_motor();

		/* Then send info to Domotix */
		digitalWrite(PIN_OUT_DOMOTIX_STATE_DOOR, DOOR_OPENED);
	    }
	    else if (g_state_current == STATE_CLOSING)
	    {
		/* check if door is blocked */
		g_current_time = now();
		if (g_current_time > (g_closing_time + BLOCKED_DOOR_TIMEOUT))
		{
		    /* something when wrong, door is blocked */
		    g_state_current  = STATE_OPENED;

		    /* Stop immediatly */
		    stop_motor();

		    /* Then send info to Domotix */
		    digitalWrite(PIN_OUT_DOMOTIX_STATE_DOOR, DOOR_OPENED);
		}
	    }
	}

	/* Check if outdoor button is pressed */
	move_the_door  = digitalRead(PIN_IN_SENSOR_ACTION_MOVE);
	if (move_the_door == 1)
	{
	    /* Wait for button released */
	    while (digitalRead(PIN_IN_SENSOR_ACTION_MOVE) == 1)
	    {
		delay(10);
	    }
	}
	else
	{
	    /* Check if door must be openned or closed by action domotix */
	    move_the_door = digitalRead(PIN_IN_DOMOTIX_ACTION_MOVE);
	    if (move_the_door == 1)
	    {
		while (digitalRead(PIN_IN_DOMOTIX_ACTION_MOVE) == 1)
		{
		    delay(10);
		}
	    }
	}

	if ((move_the_door) && ((g_state_current == STATE_CLOSED) || (g_state_current == STATE_CLOSING)))
	{
	    open_the_door();
	    g_state_current = STATE_OPENNING;
	    g_openning_time = now();
	}
	else if ((move_the_door) && ((g_state_current == STATE_OPENED) || (g_state_current == STATE_OPENNING)))
	{
	    close_the_door();
	    g_state_current = STATE_CLOSING;
	    g_closing_time = now();
	}


	/* check if it's not openning or closing for a too long time !!! */
	if (g_state_current == STATE_OPENNING)
	{
	    g_current_time = now();
	    if (g_current_time > (g_openning_time + OPENING_DOOR_TIMEOUT))
	    {
		/* Too long to open the door, then stop motor */
		/* Stop immediatly */
		g_state_current  = STATE_CLOSED;


		/* Stop immediatly */
		stop_motor();

		/* Then send info to Domotix */
		digitalWrite(PIN_OUT_DOMOTIX_STATE_DOOR, DOOR_CLOSED);
	    }
	}
	else if (g_state_current == STATE_CLOSING)
	{
	    g_current_time = now();
	    if (g_current_time > (g_closing_time + CLOSING_DOOR_TIMEOUT))
	    {
		/* Too long to close the door, then stop motor */
		g_state_current  = STATE_OPENED;

		/* Stop immediatly */
		stop_motor();

		/* Then send info to Domotix */
		digitalWrite(PIN_OUT_DOMOTIX_STATE_DOOR, DOOR_OPENED);
	    }
	}
    }
}

void process_poule(void)
{
    uint16_t poule_gauche;
    uint16_t poule_droite;

    if (g_process_poule != 0)
    {
	/* convert analog read to binary state */
	poule_gauche = analogRead(PIN_IN_SENSOR_PONDOIR_GAUCHE);
	if (poule_gauche > THRESHOLD_POULE)
	{
	    g_poule_gauche_state = PONDOIR_POULE;
	}
	else
	{
	    g_poule_gauche_state = PONDOIR_EMPTY;
	}

	/* check if it's really empty or full
	 * wait some time and check again
	 */
	if (g_poule_gauche_previous_state != g_poule_gauche_state)
	{
	    g_poule_gauche_previous_state = g_poule_gauche_state;

	    /* start counting time */
	    g_previous_poule_gauche_time = now();
	}
	else
	{
	    g_current_poule_gauche_time = now();
	    if (g_current_poule_gauche_time > (g_previous_poule_gauche_time + 240))
	    {
		digitalWrite(PIN_OUT_DOMOTIX_POULE_GAUCHE, g_poule_gauche_state);
		g_previous_poule_gauche_time = g_poule_gauche_state;
	    }
	}



	poule_droite = analogRead(PIN_IN_SENSOR_PONDOIR_DROITE);
	if (poule_droite > THRESHOLD_POULE)
	{
	    g_poule_droite_state = PONDOIR_POULE;
	}
	else
	{
	    g_poule_droite_state = PONDOIR_EMPTY;
	}

	/* check if it's really empty or full
	 * wait some time and check again
	 */
	if (g_poule_droite_previous_state != g_poule_droite_state)
	{
	    g_poule_droite_previous_state = g_poule_droite_state;

	    /* start counting time */
	    g_previous_poule_droite_time = now();
	}
	else
	{
	    g_current_poule_droite_time = now();
	    if (g_current_poule_droite_time > (g_previous_poule_droite_time + 240))
	    {
		digitalWrite(PIN_OUT_DOMOTIX_POULE_DROITE, g_poule_droite_state);
		g_previous_poule_droite_time = g_poule_droite_state;
	    }
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
    uint8_t sensorValue = 0;

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

		case 'f':
		{
		    sensorValue = analogRead(PIN_IN_SENSOR_PONDOIR_GAUCHE);

		    Serial.print("pondoir gauche = ");
		    Serial.print(sensorValue);
		    sensorValue = analogRead(PIN_IN_SENSOR_PONDOIR_DROITE);

		    Serial.print(" --- ");
		    Serial.println(sensorValue);

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
    process_door();
    process_poule();
}




