/* 

   MX2 Dolly Engine version
   
   (c) 2010-2011 C.A. Church / Dynamic Perception LLC
   (c) 2014 William B Phelps
   
   Sketch to run the "DollyShield" design, provides control of
   the arduino shield.  A dual 1Amp motor control 
   shield with integrated LCD, 5 buttons, two generic I/O ports,
   and dual opto-coupled input for camera control.  
 
   This sketch creates a user interface and implements core features
   of the shield, allowing for integrated control of one camera and 
   up to two axes of motion.  Features allow for all of the following
   motion types: shoot-move-shoot, continuous, and slow "pulsing" of 
   the full speed motor to allow for very slow speeds without more
   expensive steppers or even encoders.  
   
   For more info go to http://openmoco.org
   
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
*/

/*

  ===========================================================
  Optimized by William Phelps (tm) (c) 2014 William B. Phelps
  ===========================================================
  
  1) start motor on first request (remove mcur_spd)
  2) rewrite EE logic to use structure (saves 2400 bytes)
  3) named constants help make code more readable
  4) debounce menu buttons
  5) save motor speeds & directions
  6) separate motor speed & direction for manual move
  7) menu wrap & position remembered
  8) alternate menu: left button goes back, right selects, enter confirms changes
  9) ui_type as numeric instead of bits
  10) don't show menu when up pressed on main screen, main input 0
  11) clear pending interrupts on ext1 pins
  12) park feature (left/right)
  13) underline or block cursor
  14) change "Motor Sl.Mod" to "Motor Mode": Interleave or Continuous
  15) remove "dead spot" in main screen, return from menu always set to off/on pos
  16) backlight timeout initialized at startup
  17) new menu is default
  18) debounce ext1 input (switches) - use polling instead of interrupt
  19) put dead spot back in main screen (but only if Scope enabled)
  20) rewrite button debounce/hold logic
	21) fix ramping bug, reset motor speed display on stop
	22) improve ramp calculations for small speed changes (ramp steps > speed)
    
TODO:
  *) menu timeout?
  *) wrap on setting lists?
*/
#include <Arduino.h>
#include <LiquidCrystal.h>
#include "MsTimer2.h"
#include "TimerOne.h"
#include "merlin_mount.h"
#include "ds_eeprom.h"

#define FIRMWARE_VERSION 94
  // EE version - change this to force reset of EE memory
#define EE_VERSION 93

  // motor PWM
#define MOTOR0_P 5
#define MOTOR1_P 6
#define MOTOR0_DIR 15
#define MOTOR1_DIR 16

 // camera pins
#define CAMERA_PIN 13
#define FOCUS_PIN 12

#define MAX_MOTORS 2

//#define MP_PERIOD 30
#define MP_PERIOD 30


/* User Interface Values */

  // External switch pins
#define EXT_PIN1 2
#define EXT_PIN2 3

  // lcd pins
#define LCD_RS  17
#define LCD_EN  18
#define LCD_D4  11
#define LCD_D5  8
#define LCD_D6  7
#define LCD_D7  4

 // which input is our button
#define BUT_PIN 14

  // lcd backlight pin
#define LCD_BKL 9

  // max # of LCD characters (including newline)
#define MAX_LCD_STR 17



 // how many buttons do we have?
#define NUM_BUTTONS 5

 // button return values

#define BUT0  1
#define BUT1  2
#define BUT2  3
#define BUT3  4
#define BUT4  5

  // which buttons?
#define BUT_UP BUT4
#define BUT_DN BUT3
#define BUT_CT BUT0
#define BUT_LT BUT2
#define BUT_RT BUT1

  // analog button read values
#define BUT0_VAL  70
#define BUT1_VAL  250
#define BUT2_VAL  450
#define BUT3_VAL  655
//#define BUT3_VAL 540
#define BUT4_VAL  830

  // button variance range
#define BUT_THRESH  60

	// button debounce time
#define BUT_DEBOUNCE_MS 20 // 20 ms

 // how many ms does a button have
 // to be held before triggering another
 // action? (for scrolling, etc.)
#define BUT_HOLD_MS 500
 // hold long after that before button repeats?
#define BUT_REPEAT_MS 200

 // how much to increment for each cycle the button is held?
#define HOLD_BUT_VALINC 10


  // menu strings
prog_char menu_1[] PROGMEM = "Manual Move";
prog_char menu_2[] PROGMEM = "Axis 1";
prog_char menu_3[] PROGMEM = "Axis 2";
prog_char menu_4[] PROGMEM = "Camera";
prog_char menu_5[] PROGMEM = "Park";
prog_char menu_6[] PROGMEM = "Reset";
prog_char menu_7[] PROGMEM = "Settings";
prog_char menu_8[] PROGMEM = "Scope";

prog_char manual_menu_1[] PROGMEM = "Axis 1";
prog_char manual_menu_2[] PROGMEM = "Axis 2";
prog_char manual_menu_3[] PROGMEM = "Scope";

prog_char axis_menu_1[] PROGMEM = "Ramp Shots";
prog_char axis_menu_2[] PROGMEM = "RPM";
prog_char axis_menu_4[] PROGMEM = "Angle";
prog_char axis_menu_5[] PROGMEM = "Calibrate";
prog_char axis_menu_6[] PROGMEM = "Slow Mode IPM";
prog_char axis_menu_7[] PROGMEM = "Dist. per Rev";
prog_char axis_menu_8[] PROGMEM = "Min Pulse";
prog_char axis_menu_10[] PROGMEM = "Lead In";
prog_char axis_menu_11[] PROGMEM = "Lead Out";
prog_char axis_menu_12[] PROGMEM = "Cal. Constant";

prog_char camera_menu_1[] PROGMEM = "Interval sec";
prog_char camera_menu_2[] PROGMEM = "Max Shots";
prog_char camera_menu_3[] PROGMEM = "Exp. Time ms";
prog_char camera_menu_4[] PROGMEM = "Exp. Delay ms";
prog_char camera_menu_5[] PROGMEM = "Focus Tap ms";
prog_char camera_menu_6[] PROGMEM = "Shutter+Focus";
prog_char camera_menu_7[] PROGMEM = "Repeat";
prog_char camera_menu_8[] PROGMEM = "Repeat Delay";

prog_char set_menu_1[] PROGMEM = "Alt Menu";
prog_char set_menu_2[] PROGMEM = "Motor Disp";
prog_char set_menu_3[] PROGMEM = "Motor Mode";
prog_char set_menu_4[] PROGMEM = "Backlight";
prog_char set_menu_5[] PROGMEM = "AutoDim (sec)";
prog_char set_menu_6[] PROGMEM = "Blank LCD";
prog_char set_menu_7[] PROGMEM = "I/O 1";
prog_char set_menu_8[] PROGMEM = "I/O 2";
prog_char set_menu_9[] PROGMEM = "Metric Disp.";
prog_char set_menu_10[] PROGMEM = "Scope";
prog_char set_menu_11[] PROGMEM = "Cal. Spd Low";
prog_char set_menu_12[] PROGMEM = "Cal. Spd Hi";
prog_char set_menu_13[] PROGMEM = "AltOut Pre ms";
prog_char set_menu_14[] PROGMEM = "AltOut Post ms";
prog_char set_menu_15[] PROGMEM = "USB Trigger";
prog_char set_menu_16[] PROGMEM = "Invert Dir";
prog_char set_menu_17[] PROGMEM = "Invert I/O";
prog_char set_menu_18[] PROGMEM = "ULine Cursor"; // underline cursor

prog_char park_menu_1[] PROGMEM = "Park Left";
prog_char park_menu_2[] PROGMEM = "Park Right";
//prog_char park_menu_3[] PROGMEM = "Park Middle";

prog_char reset_menu_1[] PROGMEM = "Reset Cal";
prog_char reset_menu_2[] PROGMEM = "Reset Mem";

prog_char scope_menu_1[] PROGMEM = "Pan Man. Spd.";
prog_char scope_menu_2[] PROGMEM = "Tilt Man. Spd.";

 // menu organization

PROGMEM const char *menu_str[]  = { menu_1, menu_2, menu_3, menu_4, menu_5, menu_6, menu_7, menu_8 };

PROGMEM const char *man_str[]   = { manual_menu_1, manual_menu_2, manual_menu_3 };

PROGMEM const char *park_str[]   = { park_menu_1, park_menu_2 };

PROGMEM const char *axis0_str[] = { axis_menu_1, axis_menu_10, axis_menu_11, axis_menu_2, axis_menu_4, axis_menu_5, axis_menu_12, axis_menu_6, axis_menu_7, axis_menu_8 };
PROGMEM const char *axis1_str[] = { axis_menu_1, axis_menu_10, axis_menu_11, axis_menu_2, axis_menu_4, axis_menu_5, axis_menu_12, axis_menu_6, axis_menu_7, axis_menu_8 };
PROGMEM const char *cam_str[]   = { camera_menu_1, camera_menu_2, camera_menu_3, camera_menu_4, camera_menu_5, camera_menu_6, camera_menu_7, camera_menu_8 };
PROGMEM const char *set_str[]   = { set_menu_1, set_menu_2, set_menu_3, set_menu_4, set_menu_5, set_menu_6, set_menu_7, set_menu_8, set_menu_9, set_menu_10, set_menu_11, set_menu_12, set_menu_13, set_menu_14, set_menu_15, set_menu_16, set_menu_17, set_menu_18 };
PROGMEM const char *scope_str[] = { scope_menu_1, scope_menu_2 };
PROGMEM const char *reset_str[] = { reset_menu_1, reset_menu_2 };

  // max number of inputs for each menu (in order listed above, starting w/ 0)
byte max_menu[9]  = {7,2,9,9,7,1,1,17,1};

  // support a history of menus visited up to 5 levels deep
byte hist_menu[5] = {0,0,0,0,0};
byte hist_menu_cur[5] = {0,0,0,0,0}; // also save the cursor position

char lcd_buf[MAX_LCD_STR];

  // what is our currently selected menu?
  // what is our current position?
byte cur_menu      = 0;
byte cur_pos       = 0;
byte cur_pos_sel   = 0;

#define MU_MAIN     0
#define MU_MANUAL   1
#define MU_AXIS1    2
#define MU_AXIS2    3
#define MU_CAMERA   4
#define MU_PARK     5
#define MU_RESET    6
#define MU_SETTINGS 7
#define MU_SCOPE    8
#define MU_SPECIAL  254
#define MU_NOTARGET 255

  // input buffers
unsigned long cur_inp_long  = 0;
float cur_inp_float         = 0.0;
boolean cur_inp_bool        = false;

  // which input are we on, if on
  // the main screen.
char main_scr_input         = 1;

  // last read button (analog) value
int last_but_rd = 1013;  // (out of range value)

  // flags for each button
  // use indivial bits to indicate
  // whether a given button was pressed.
  
///byte button_pressed = 0;

  // input value multiplier
  
unsigned int inp_val_mult = 1;

	// when was button first pressed? (for debounce)
unsigned long but_push_tm = 0;
  // how long has a button been held for?
///unsigned long but_hold_tm = 0;
  // when was ui last updated on home scr?
unsigned long ui_update_tm = 0;
  // when was cursor blinked last?
unsigned long ui_blink_tm = 0;

 // for dimming lcd
unsigned long input_last_tm = 0;
 
  // floats are input in tenths?
boolean ui_float_tenths = false;

 /* user interface control flags
 
   B0 = update display
   B1 = currently in setup menu
   B2 = in value entry
   B3 = have drawn initial value in value entry
   B4 = have drawn decimal in current value
   B5 = in manual mode
   B6 = lcd bkl on
   B7 = in calibrate mode
   
 */
   
#define UC_Update         B10000000  // display update needed
#define UC_Setup          B01000000
#define UC_Value_Entry    B00100000
#define UC_Value_Drawn    B00010000
//#define UC_Value_Decimal  B00001000 not used
#define UC_Park           B00001000
#define UC_Manual         B00000100
#define UC_Backlight_On   B00000010
#define UC_Calibrate      B00000001
   
byte ui_ctrl_flags = B00000000;

 /* calibration screen flags
 
    B0 = Currently calibrating
    B1 = Done Calibrating
    
 */

#define US_Calibrate     B10000000
#define US_CalibrateDone B01000000
 
byte ui_cal_scrn_flags = 0;

	// UI Types
#define UT_Integer   0  // input is an int (default case)
#define UT_OnOff     1  // input is boolean (on/off)
#define UT_UpDn      2  // input is boolean (up/dn)
#define UT_LtRt      3  // input is boolean (lt/rt)
#define UT_IpmPct    4  // input is boolean (ipm/pct)
#define UT_ContSms   5  // input is boolean (continuous/sms)
#define UT_RotLin    6  // input is boolean (rotary/lineary)

#define UT_Boolean   6  // all values between 1 and this are boolean

#define UT_Float     7  // input is a float
#define UT_Angle     8  // input is a list (0,45,90) angle
#define UT_AltIO     9  // input is a list (disable/start/stop...)
#define UT_CalSpeed 10  // input is motor speed cal
#define UT_Park     11  // input is a list (stop/run/done) park
 
byte ui_type = 0;

	// Alt I/O Input types (for external input - ext1, ext2)
#define IT_Disabled  0
#define IT_Start     1
#define IT_Stop      2
#define IT_Toggle    3
#define IT_ExtInterval 4
#define IT_OutBefore 5
#define IT_OutAfter  6
#define IT_OutBoth   7
#define IT_ChangeDir 8


  /* run status flags
  
    B0 = running
    B1 = camera currently engaged
    B2 = camera cycle complete
    B3 = motors currently running
    B4 = external trigger engaged
    B5 = parking
    
  */
#define RS_Running          B10000000
#define RS_Camera_Active    B01000000
#define RS_Camera_Complete  B00100000
#define RS_Motors_Running   B00010000
#define RS_External_Trigger B00001000
#define RS_Parking          B00000100

volatile byte run_status = 0;

  /* external intervalometer
    
    B0 = I/O 1 is external intervalometer
    B1 = I/O 2 is external intervalometer
    B2 = interval OK to fire
  
  */
  
byte external_interval = 0;

 /* external trigger via alt i/o pins
 
  B0 = I/O 1 external enabled (before)
  B1 = I/O 2 external enabled (before)
  B2 = I/O 1 external enabled (after)
  B3 = I/O 2 external enabled (after)

 */
  
byte external_trigger  = 0;

byte pre_focus_clear      = 0;
unsigned long cam_last_tm = 0;

  // currently selected motor
byte cur_motor = 0;
 // motor mode, auto or manual?
byte m_mode = 0;
  // currently set speed (for altering motor speed)
unsigned int m_speeds[2][2] = {0,0};  // current motor speeds, auto & manual
  // current state of each motor - to show on lcd if motor running
byte m_state[2] = {0,0}; // wbp


 // for timer1 pulsing mode control
boolean timer_used = false;
volatile  bool timer_engaged      = false;
volatile bool motor_engaged      = false;
volatile byte motor_ran = 0;

 // motor calibration
byte m_cur_cal = 0;
boolean m_cal_done = false;

 // ramping data
float m_ramp_shift[2]  = {0.0,0.0};

 // for controlling pulsing and sms movement
unsigned long on_pct[2]                = {0,0};
unsigned long off_pct[2]               = {0,0};
unsigned int m_sms_tm[2]              = {0,0};

 // shots fired
unsigned long shots = 0;

 // function types for alt inputs...
 
 /* 
   0 = disabled
   1 = start
   2 = stop
 */
 
	// time of last external trigger state change
unsigned long ext_trigger_tm[2] = {0,0}; 
bool ext_trigger_last[2] = {false,false};
bool ext_trigger_state[2] = {false,false};
bool input_trigger[2] = {false,false};
#define TRIGGER_DEBOUNCE_MS 200  // a bit more conservative than button debounce


byte  merlin_dir[2]        = {0,0};
byte  merlin_wasdir[2]     = {0,0};
float merlin_speeds[2]     = {0.0,0.0};

 /*
   
   0 = axis 0 currently running free (continuous)
   1 = axis 1 currently running free
   2 = in merlin manual control
   3 = displaying merlin config screen
   
*/

byte merlin_flags = 0;

 // initialize LCD object
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);


void setup() { 

  pinMode(CAMERA_PIN, OUTPUT);
  pinMode(FOCUS_PIN, OUTPUT);
  pinMode(MOTOR0_P, OUTPUT);
  pinMode(MOTOR1_P, OUTPUT);
  pinMode(MOTOR1_DIR, OUTPUT);
  pinMode(MOTOR0_DIR, OUTPUT);

  Serial.begin(9600); 

  init_user_interface();
//  delay(1000);
  
  ee_load(); // load variables saved in EE (load defaults if needed)
   
   // if ext1 input set, attach (or detach) interrupts
  altio_connect(0, EE.input_type[0]);
  altio_connect(1, EE.input_type[0]);
  
  // set lcd backlight to saved value
  ui_set_backlight(EE.cur_bkl);
  
  setBlink(true); // must come after ee_load
  input_last_tm = millis(); // start the backlight timer

  show_home();

/*
  // print calibration data for motor 0
  for (byte a = 0; a <= 2; a++) { // angle
    Serial.print("angle: ");
		Serial.println(a, DEC);
		for( byte i = 0; i <= 2; i++) {
			Serial.print(i, DEC);
			Serial.print(": ");
			for ( byte x = 0; x < 2; x++ ) {
				Serial.print(EECal.m_cal_array[0][a][i][x], 8);
				Serial.print(":");
			}
			Serial.println("");
		}
	}
	Serial.print("cal const: ");
	for( byte i = 0; i < 2; i++) {
//		Serial.print(i, DEC);
//		Serial.print(": ");
		Serial.print(EECal.m_cal_constant[i], 8);
		Serial.print(", ");
	}
	Serial.println("");
	Serial.print("cal speed: ");
	for( byte i = 0; i < 2; i++) {
//		Serial.print(i, DEC);
//		Serial.print(": ");
		Serial.print(EECal.m_cal_speed[i], 8);
		Serial.print(", ");
	}
	Serial.println("");
 */

}



void loop() {

      // check for signal from gbtimelapse serial command.
      // we check here to prevent queuing commands when stopped
      
  if( EE.gb_enabled == true && gbtl_trigger() == true ) {
    external_interval |= B00100000;
  }

  if( run_status & RS_Running ) {
    // program is running
    main_loop_handler();
  } // end if running
  else {
      // not running, not in manual, but merlin motors are running?
    if( ! ( ui_ctrl_flags & UC_Manual ) && merlin_flags & B11000000 ) {
      merlin_stop(0);
      merlin_stop(1);
    }
  }
  
    // always check the UI for input or updates
  check_user_interface();    
 
}




void main_loop_handler() {
  
  
  static boolean camera_fired   = false;
  static boolean motors_clear   = false;
  static boolean ok_stop        = false;
  static boolean in_sms_cycle   = false;
  static boolean do_fire        = false;
  static boolean ext_trip       = false;
  static byte    cam_repeated   = 0;
  
  
  if( EE.cam_max > 0 && shots >= EE.cam_max && ( ok_stop || (m_speeds[0][0] <= 0.0 && m_speeds[0][1] <= 0.0) || EE.motor_mode ) ) {
    
     // stop program if max shots exceeded, and complete cycle completed
     // if in interleave, ignore complete cycle if in pulse
   ok_stop = false;
   stop_executing();
      // interrupt further processing      
  }
  else if( pre_focus_clear == 2 ) {  
        // allow 100ms for focus line to settle before allowing any further
      MsTimer2::set(100, clear_cam_focus);
      MsTimer2::start();
      pre_focus_clear = 3;
  }
  else  if( EE.motor_mode &&
            (( m_speeds[0][0] > 0 && m_speeds[0][0] < EE.min_spd[0] ) ||
             ( m_speeds[0][1] > 0 && m_speeds[0][1] < EE.min_spd[1] ) ) ) {
        
      // if pulse mode is on and at least
      // one motor needs to be pulsed...
    
    motor_run_pulsing();
        
  }
  
    // run merlin in continuous mode if needed
    
  if( EE.motor_mode && EE.merlin_enabled ) {
    merlin_run_cont();
    ok_stop = true; // allow max shots stop when
                    // in continuous mode
  }
  
   
    // we need to deterime if we can shoot the camera
    // by seeing if it is currently being fired, or 
    // is blocked in some way.  After making sure we're
    // not blocked, we check to see if its time to fire the
    // camera

  if( motor_engaged ) {      
    // motor currently moving
    // do not fire camera until motors are
    // done - we get caught up in here until the
    // motors have stopped moving
    
    if( motor_ran == 1 ) {
      // check to see if motor #2 needs to be run...
      if( in_sms_cycle == false && m_sms_tm[0] > 0 && m_sms_tm[1] > 0 ) {
        in_sms_cycle = true;
          // motor #2 remains to be run
        MsTimer2::set(m_sms_tm[1], stop_motor_sms);
        run_motor_sms(1);
        MsTimer2::start();
      } 
       else if(in_sms_cycle == false) {         
           // no motors remain
         motor_engaged = false;
         ok_stop       = true;
       }
    }
    else if ( motor_ran > 0 ) {
        // all of our motors have run one
        // cycle, let the camera fire
       motor_engaged = false;
       ok_stop       = true;
       in_sms_cycle  = false;
    }
      
  } // end if motor_engaged
  else if( run_status & (RS_Camera_Active+RS_External_Trigger) || pre_focus_clear == 3 ) {
      // currently firing the camera, focus, or triggering an external
      // control line
      
      // do nothing
    ;
  }
  else if( run_status & RS_Camera_Complete ) {
      // camera cycle completed
        // clear exposure cycle complete flag
    run_status &= (255-RS_Camera_Complete);
    if( camera_fired == true ) {
      // the shot just fired
      camera_fired = false;
      shots++;
      
      
        // for ramping motor speeds
        // we change speed in ramps after shots...

      motor_execute_ramp_changes();
        // check to see if a post-exposure delay is needed
      
      if( EE.post_delay_tm > 0 ) {
          // we block anything from happening while in the
          // post-exposure cycle by pretending to be an
          // exposure
        run_status |= RS_Camera_Active;
        
        MsTimer2::set(EE.post_delay_tm, camera_clear);
        MsTimer2::start();
        
        motors_clear = false;
        ok_stop = false;
      }
      else {
          // no post-exp delay, is the external trigger to fire?
        if( external_trigger & B00110000 && EE.ext_trig_pst_delay > 0 )
          alt_ext_trigger_engage(false);
       

        //no post-exposure delay, motors can run
        motors_clear = true;
      }
        
    } 
    else {
        // this was a post-exposure delay cycle completing, not
        // an actual shot
        
        // is the external trigger to fire?
      if( external_trigger & B00110000 && EE.ext_trig_pst_delay > 0 )
        alt_ext_trigger_engage(false);
      

        // we can set the motors clear to move now
      motors_clear = true;        
    }
    
        // is the merlin head set to move?    
    if( motors_clear == true && EE.merlin_enabled  && ! EE.motor_mode ) {
        // send merlin head to move sms distances (if desired)             
  
      if( merlin_speeds[0] > 0.0 ) {        
        merlin_send_angle(0, merlin_speeds[0]);
        ok_stop = false;
      }
      if( merlin_speeds[1] > 0.0 ) {
        merlin_send_angle(1, merlin_speeds[1]);
        ok_stop = false;
      }
      
    }

  }
  else if( motors_clear == true && ! EE.motor_mode && 
            ( EE.merlin_enabled && ( merlin_flags & B11000000 ) ) ) {
    
        // merlin motors are currently running!
        // other actions cannot continue...
        
    if( ! merlin.readAxisStatus(1) && ! merlin.readAxisStatus(2) ) {
        // but not actually running... now remove the flags
        // indicating so.
      merlin_flags &= B00111111;
      ok_stop = true;
    }  
    
  } 
  else if( motors_clear == true && ! EE.motor_mode &&
              ( m_sms_tm[0] > 0 || m_sms_tm[1] > 0 ) ) {
      
       // if we're set to go to s-m-s and at least one motor is set to move
      // start DC motor(s) moving
  
      motor_ran = 0;
  
        // set motors to move, and then
        // set timer to turn them off  
        
      if( m_sms_tm[0] > 0 ) {
            // start first motor
        run_motor_sms(0); 
        MsTimer2::set(m_sms_tm[0], stop_motor_sms);
      }
      else if( m_sms_tm[1] > 0 ) {
          // start second motor (see state
          // handling above, where motor_engaged is set)
        run_motor_sms(1); 
        MsTimer2::set(m_sms_tm[1], stop_motor_sms);
      }
  
        // engage timer
      MsTimer2::start();
      
      motor_engaged = true;
      motors_clear = false;
      ok_stop      = false;
    
  }   
  else if( EE.gb_enabled == true || external_interval & B11000000 ) {
    // external intervalometer is engaged
    
    if( external_interval & B00100000 ) {
      // external intervalometer has triggered
 
          // clear out ok to fire flag
      external_interval &= B11011111;      
      do_fire = true;
    }
  }
  else if( cam_last_tm < millis() - (EE.cam_interval * 1000) ) {
      // internal intervalometer triggers
    do_fire = true;
  }
  
  if( do_fire == true ) {
      // we've had a fire camera event
    
           // is the external trigger to fire? (either as 'before' or 'through')
    if( external_trigger & B11000000 && EE.ext_trig_pre_delay > 0 && ext_trip == false && (EE.cam_repeat == 0 || cam_repeated == 0) ) {
        alt_ext_trigger_engage(true);
        ext_trip = true;
    }
    else {

        // make sure we handle pre-focus tap timing
        
      if( ( pre_focus_clear == 4 || EE.focus_tap_tm == 0 || (EE.cam_repeat > 0 && cam_repeated > 0) ) && !(run_status & RS_External_Trigger) ) {
  
          // we always set the start mark at the time of the
          // first exposure in a repeat cycle (or the time of exp
          // if no repeat cycle is in play
          
        if( EE.cam_repeat == 0 || cam_repeated == 0 )
          cam_last_tm  = millis();
  
          // deal with camera repeat actions
        if( EE.cam_repeat == 0 || (EE.cam_repeat > 0  && cam_repeated >= EE.cam_repeat) ) {
          camera_fired = true;
          do_fire = false;
          ext_trip = false;
          cam_repeated = 0;
        }
        else if( EE.cam_repeat > 0 ) {
            // only delay ater the first shot
          if( cam_repeated > 0 )
            delay(EE.cam_rpt_dly); // blocking delay between camera firings (we should fix this later!)
            
          cam_repeated++;
        }
        
        // camera is all clear to fire, and enough
        // time is elapsed
        fire_camera(EE.exp_tm);
        pre_focus_clear = 0;
        
      }
      else if( EE.focus_tap_tm > 0 && pre_focus_clear == 0 && !(run_status & RS_External_Trigger) ) {
          // pre-focus tap is set, bring focus line high
        digitalWrite(FOCUS_PIN, HIGH);
        MsTimer2::set(EE.focus_tap_tm, stop_cam_focus);
        MsTimer2::start();
        pre_focus_clear = 1;
      }
    } // end else (not external trigger...
  } // end if(do_fire...
}


void start_executing() {
  // starts program execution
  
   // clear out external interval flag in case it was set while stopped.
    
  external_interval &= B11011111;
  
  run_status |= (RS_Running+RS_Motors_Running);
  
    // set motor direction
  motor_dir(0, EE.m_dirs[m_mode][0]);
  motor_dir(1, EE.m_dirs[m_mode][1]);
    // (re)set motor speed
  motor_set_speed(0,EE.m_speeds[m_mode][0]);
  motor_set_speed(1,EE.m_speeds[m_mode][1]);
    // calculate speed change per shot for ramping
    // if needed - use function to update values
  motor_set_ramp(0, EE.m_ramp_set[0]);
  motor_set_ramp(1, EE.m_ramp_set[1]);
  
  // turn on motors
  motor_control(0,true);
  motor_control(1,true);
  
    // if ramping is enabled for a motor, start at a zero speed
  if( EE.m_ramp_set[0] >= 1 )
      motor_set_speed(0, 0); 
  if( EE.m_ramp_set[1] >= 1 )
      motor_set_speed(1, 0); 

    // reset shot counter
  shots = 0;
}

void stop_executing() {
  run_status &= (255-RS_Running-RS_Motors_Running);
  motor_stop_all();
	  // restore motor speeds in case we were ramping
  m_speeds[m_mode][0] = EE.m_speeds[m_mode][0];
  m_speeds[m_mode][1] = EE.m_speeds[m_mode][1];
}


boolean gbtl_trigger() {
  
  if( Serial.available() > 0 ) {
    char thsChar = Serial.read();
    
    if( thsChar == 'T' ) {
      return true;
    }
    else {
      return false;
    }
  }
  
  return false;
}
