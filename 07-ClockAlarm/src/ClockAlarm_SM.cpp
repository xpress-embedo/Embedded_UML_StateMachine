/*.$file${HSMs::../src::ClockAlarm_SM.cpp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*
* Model: ClockAlarm.qm
* File:  ${HSMs::../src::ClockAlarm_SM.cpp}
*
* This code has been generated by QM 5.1.1 <www.state-machine.com/qm/>.
* DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*/
/*.$endhead${HSMs::../src::ClockAlarm_SM.cpp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
#include <Arduino.h>
#include "qpn.h"
#include "lcd.h"
#include "ClockAlarm_SM.h"

/* Some Helper macros */
#define GET_HOUR(seconds)     ( seconds/3600ul )
#define GET_MIN(seconds)      ( (seconds/60ul) % 60ul )
#define GET_SEC(seconds)      ( seconds % 60ul )
#define DIGIT1(d)       	  ( d / 10u)
#define DIGIT2(d)       	  ( d % 10u)

/*.$declare${HSMs::Clock_Alarm} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*.${HSMs::Clock_Alarm} ....................................................*/
typedef struct Clock_Alarm {
/* protected: */
    QHsm super;

/* private: */
    uint32_t temp_time;
    uint32_t alarm_time;
    uint8_t alarm_status;
    uint8_t time_mode;

/* private state histories */
    QStateHandler hist_Clock;
} Clock_Alarm;

/* public: */
static uint32_t Clock_Alarm_GetCurrentTime(void);
static void Clock_Alarm_UpdateCurrentTime(void);
static void Clock_Alarm_SetCurrentTime(uint32_t new_current_time);

/*
 * Description : Displays current time depending upon the time mode
 * param1: 'me' pointer
 * param2 : row number of the LCD
 * param3: column number of the LCD
 */
static void Clock_Alarm_DisplayCurrentTime(Clock_Alarm * const me, uint8_t row, uint8_t col);
extern uint32_t Clock_Alarm_current_time;
extern Clock_Alarm Clock_Alarm_obj;

/* protected: */
static QState Clock_Alarm_initial(Clock_Alarm * const me);
static QState Clock_Alarm_Clock(Clock_Alarm * const me);
static QState Clock_Alarm_Ticking(Clock_Alarm * const me);
static QState Clock_Alarm_Settings(Clock_Alarm * const me);
static QState Clock_Alarm_Clock_Setting(Clock_Alarm * const me);
static QState Clock_Alarm_Alarm_Setting(Clock_Alarm * const me);
static QState Clock_Alarm_Alarm_Notify(Clock_Alarm * const me);
/*.$enddecl${HSMs::Clock_Alarm} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/* Helper Function Prototypes */
String GetAM_PM( uint32_t time24h );
void display_write( String str_, uint8_t r, uint8_t c );
String IntegerTime_ToString( uint32_t time_ );
uint32_t Convert12H_To_24H( uint32_t time12h, time_format_t am_pm );
uint32_t Convert24H_To_12H( uint32_t time24h );

/*.$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*. Check for the minimum required QP version */
#if (QP_VERSION < 690U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpn version 6.9.0 or higher required
#endif
/*.$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*.$define${HSMs::Clock_Alarm_ctor} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*.${HSMs::Clock_Alarm_ctor} ...............................................*/
void Clock_Alarm_ctor(void) {
    QHsm_ctor( &Clock_Alarm_obj.super, Q_STATE_CAST(Clock_Alarm_Ticking));
}
/*.$enddef${HSMs::Clock_Alarm_ctor} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/*.$define${HSMs::Clock_Alarm} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*.${HSMs::Clock_Alarm} ....................................................*/
uint32_t Clock_Alarm_current_time;
Clock_Alarm Clock_Alarm_obj;
/*.${HSMs::Clock_Alarm::GetCurrentTime} ....................................*/
static uint32_t Clock_Alarm_GetCurrentTime(void) {
    uint32_t temp = 0u;
    noInterrupts();
    temp = Clock_Alarm_current_time;
    interrupts();
    return (temp);
}

/*.${HSMs::Clock_Alarm::UpdateCurrentTime} .................................*/
static void Clock_Alarm_UpdateCurrentTime(void) {
    /* this function is called from ISR hence we don't need to disable and enable
    interrupts to update this attribute of the class */
    Clock_Alarm_current_time++;
    /* If maximum value is reached, reset the current time */
    if( Clock_Alarm_current_time >= MAX_TIME )
    {
      Clock_Alarm_current_time = 0u;
    }
}

/*.${HSMs::Clock_Alarm::SetCurrentTime} ....................................*/
static void Clock_Alarm_SetCurrentTime(uint32_t new_current_time) {
    /* current_time is also getting updated in interrupts, hence we can't update
    it directly, it should be an atomic update */
    noInterrupts();
    Clock_Alarm_current_time = new_current_time;
    interrupts();
}


/*
 * Description : Displays current time depending upon the time mode
 * param1: 'me' pointer
 * param2 : row number of the LCD
 * param3: column number of the LCD
 */
/*.${HSMs::Clock_Alarm::DisplayCurrentTime} ................................*/
static void Clock_Alarm_DisplayCurrentTime(Clock_Alarm * const me, uint8_t row, uint8_t col) {
    String time_as_string;
    uint32_t time_;

    /* convert to number of seconds */
    uint32_t time24h = Clock_Alarm_GetCurrentTime()/10;
    /* extract sub-second to append later */
    uint8_t ss = time24h % 10U;

    if( me->time_mode == TIME_MODE_24H )
    {
      /* already in 24 hour format */
      time_ = time24h;
    }
    else
    {
      time_ = Convert24H_To_12H( time24h );
    }

    /* convert integer time to string in hh:mm:ss format*/
    time_as_string = IntegerTime_ToString( time_ );
    /* concatenate sub-seconds */
    time_as_string.concat('.');
    time_as_string.concat( ss );

    /* if mode is 12H , concatenate  am/pm information */
    if( me->time_mode == TIME_MODE_12H )
    {
      time_as_string.concat(' ');
      time_as_string.concat( GetAM_PM(time24h) );
    }
    /* update display */
    display_write(time_as_string, row, col);
}

/*.${HSMs::Clock_Alarm::SM} ................................................*/
static QState Clock_Alarm_initial(Clock_Alarm * const me) {
    /*.${HSMs::Clock_Alarm::SM::initial} */
    /* Set the current time at start-up */
    Clock_Alarm_SetCurrentTime( INITIAL_CURRENT_TIME );
    /* Also set the alarm time at start-up */
    me->alarm_time = INITIAL_ALARM_TIME;
    /* Also set the time-mode */
    me->time_mode = TIME_MODE_12H;
    /* Also set the alarm status to off at start-up */
    me->alarm_status = ALARM_OFF;
    /* state history attributes */
    /* state history attributes */
    me->hist_Clock = Q_STATE_CAST(&Clock_Alarm_Ticking);
    return Q_TRAN(&Clock_Alarm_Ticking);
}
/*.${HSMs::Clock_Alarm::SM::Clock} .........................................*/
static QState Clock_Alarm_Clock(Clock_Alarm * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*.${HSMs::Clock_Alarm::SM::Clock} */
        case Q_EXIT_SIG: {
            /* save deep history */
            me->hist_Clock = QHsm_state(me);
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*.${HSMs::Clock_Alarm::SM::Clock::Ticking} ................................*/
static QState Clock_Alarm_Ticking(Clock_Alarm * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*.${HSMs::Clock_Alarm::SM::Clock::Ticking} */
        case Q_ENTRY_SIG: {
            Clock_Alarm_DisplayCurrentTime( me, TICKING_CURR_TIME_ROW, TICKING_CURR_TIME_COL );
            status_ = Q_HANDLED();
            break;
        }
        /*.${HSMs::Clock_Alarm::SM::Clock::Ticking::SET} */
        case SET_SIG: {
            status_ = Q_TRAN(&Clock_Alarm_Clock_Setting);
            break;
        }
        /*.${HSMs::Clock_Alarm::SM::Clock::Ticking::OK} */
        case OK_SIG: {
            status_ = Q_TRAN(&Clock_Alarm_Alarm_Setting);
            break;
        }
        default: {
            status_ = Q_SUPER(&Clock_Alarm_Clock);
            break;
        }
    }
    return status_;
}
/*.${HSMs::Clock_Alarm::SM::Clock::Settings} ...............................*/
static QState Clock_Alarm_Settings(Clock_Alarm * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*.${HSMs::Clock_Alarm::SM::Clock::Settings::OK} */
        case OK_SIG: {
            status_ = Q_TRAN(&Clock_Alarm_Ticking);
            break;
        }
        /*.${HSMs::Clock_Alarm::SM::Clock::Settings::ABRT} */
        case ABRT_SIG: {
            status_ = Q_TRAN(&Clock_Alarm_Ticking);
            break;
        }
        default: {
            status_ = Q_SUPER(&Clock_Alarm_Clock);
            break;
        }
    }
    return status_;
}
/*.${HSMs::Clock_Alarm::SM::Clock::Settings::Clock_Setting} ................*/
static QState Clock_Alarm_Clock_Setting(Clock_Alarm * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        default: {
            status_ = Q_SUPER(&Clock_Alarm_Settings);
            break;
        }
    }
    return status_;
}
/*.${HSMs::Clock_Alarm::SM::Clock::Settings::Alarm_Setting} ................*/
static QState Clock_Alarm_Alarm_Setting(Clock_Alarm * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        default: {
            status_ = Q_SUPER(&Clock_Alarm_Settings);
            break;
        }
    }
    return status_;
}
/*.${HSMs::Clock_Alarm::SM::Alarm_Notify} ..................................*/
static QState Clock_Alarm_Alarm_Notify(Clock_Alarm * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*.${HSMs::Clock_Alarm::SM::Alarm_Notify::OK} */
        case OK_SIG: {
            status_ = Q_TRAN_HIST(me->hist_Clock);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*.$enddef${HSMs::Clock_Alarm} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/*
 * Description : Decodes AM/PM information from given time in 24H format
 * param1: Integer time in 24H format
 * return : A string value("AM" or "PM")
 */
String GetAM_PM( uint32_t time24h )
{
    String am_pm;
    uint8_t hour = GET_HOUR( time24h );
    if(hour == 0U)
    {
        am_pm = "AM";
    }
    else if( hour > 12U)
    {
        am_pm = "PM";
    }
    else if ( hour == 12U )
    {
        am_pm = "PM";
    }
    else
    {
        am_pm = "AM";
    }

    return am_pm;
}

/*
 * Description: Writes a message to the LCD at given row and column number
 * param1 : Message to write in 'String' format
 * param2 : row number of the LCD
 * param2 : column number of the LCD
 */
void display_write( String str_, uint8_t r, uint8_t c )
{
    lcd_set_cursor( c,r );
    lcd_print_string( str_ );
}

/*
 * Description: converts an 'integer' time to 'String' time
 * param1 : time represented in terms of number of seconds
 * return : time as 'String' value in the format HH:MM:SS
 */
String IntegerTime_ToString( uint32_t time_ )
{
    uint8_t hour, minute, second;
    char buffer[10];       // 00:00:00+null
    hour   = GET_HOUR(time_);   /* Extract how many hours the 'time_' represent */
    minute = GET_MIN(time_);    /* Extract how many minutes the 'time_' represent */
    second = GET_SEC(time_);	/* Extract how many seconds the 'time_' represent */
    sprintf( buffer, "%02d:%02d:%02d", hour, minute, second );
    return (String)buffer;
}

/*
 * Description: Converts given integer time in 12H format to integer time 24H format
 * param1 : Integer time in 12H format
 * param2 : time format of type time_format_t
 * return : Integer time in 24H format
 */
uint32_t Convert12H_To_24H( uint32_t time12h, time_format_t am_pm )
{
    uint8_t hour;
    uint32_t time24h;
    hour = GET_HOUR( time12h );
    if(am_pm == FORMAT_AM )
    {
        time24h = (hour == 12) ? (time12h-(12UL * 3600UL)) : time12h;
    }
    else
    {
        time24h = (hour == 12)? time12h : (time12h +(12UL * 3600UL));
    }
    return time24h;
}

/*
 * Description: Converts given integer time in 24H format to integer time 12H format
 * param1 : Integer time in 24H format
 * return : Integer time in 12H format
 */
uint32_t Convert24H_To_12H( uint32_t time24h )
{
    uint8_t hour;
    uint32_t time12h;
    hour = GET_HOUR(time24h);

    if(hour == 0)
    {
        time12h = time24h + (12UL * 3600UL);
    }
    else
    {
        if( (hour < 12UL) || (hour == 12UL) )
        {
            return time24h;
        }
        else
        {
            time12h = time24h - (12UL * 3600UL);
        }
    }
    return time12h;
}

ISR( TIMER1_COMPA_vect )
{
  Clock_Alarm_UpdateCurrentTime();
}
