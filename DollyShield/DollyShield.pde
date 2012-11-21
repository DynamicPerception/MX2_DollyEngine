/* 

   MX2 Dolly Engine version
   
   (c) 2010-2011 C.A. Church / Dynamic Perception LLC
   
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


#include <avr/pgmspace.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include "MsTimer2.h"
#include "TimerOne.h"
#include "merlin_mount.h";


#define FIRMWARE_VERSION  835

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



 // how many buttons dow we have?
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


 // how many ms does a button have
 // to be held before triggering another
 // action? (for scrolling, etc.)
 
#define HOLD_BUT_MS 200

 // how much to increment for each cycle the button is held?
 
#define HOLD_BUT_VALINC 10

 // ALT input debouncing time
 
#define ALT_TRIG_THRESH 250


  // menu strings
prog_char menu_1[] PROGMEM = "Manual Move";
prog_char menu_2[] PROGMEM = "Axis 1";
prog_char menu_3[] PROGMEM = "Axis 2";
prog_char menu_4[] PROGMEM = "Camera";
prog_char menu_5[] PROGMEM = "Settings";
prog_char menu_6[] PROGMEM = "Scope";

prog_char manual_menu_1[] PROGMEM = "Axis 1";
prog_char manual_menu_2[] PROGMEM = "Axis 2";
prog_char manual_menu_3[] PROGMEM = "Scope";

prog_char axis_menu_1[] PROGMEM = "Ramp Shots";
prog_char axis_menu_2[] PROGMEM = "RPM";
prog_char axis_menu_3[] PROGMEM = "Fixed SMS";
prog_char axis_menu_4[] PROGMEM = "Angle";
prog_char axis_menu_5[] PROGMEM = "Calibrate";
prog_char axis_menu_6[] PROGMEM = "Slow Mode IPM";
prog_char axis_menu_7[] PROGMEM = "Dist. per Rev";
prog_char axis_menu_8[] PROGMEM = "Min Pulse";
prog_char axis_menu_9[] PROGMEM = "Axis Type";
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

prog_char set_menu_1[] PROGMEM = "Motor Disp";
prog_char set_menu_2[] PROGMEM = "Motor Sl.Mod";
prog_char set_menu_3[] PROGMEM = "Backlight";
prog_char set_menu_4[] PROGMEM = "AutoDim (sec)";
prog_char set_menu_5[] PROGMEM = "Blank LCD";
prog_char set_menu_6[] PROGMEM = "I/O 1";
prog_char set_menu_7[] PROGMEM = "I/O 2";
prog_char set_menu_8[] PROGMEM = "Metric Disp.";
prog_char set_menu_9[] PROGMEM = "Reset Mem";
prog_char set_menu_10[] PROGMEM = "Scope";
prog_char set_menu_11[] PROGMEM = "Cal. Spd Low";
prog_char set_menu_12[] PROGMEM = "Cal. Spd Hi";
prog_char set_menu_13[] PROGMEM = "AltOut Pre ms";
prog_char set_menu_14[] PROGMEM = "AltOut Post ms";

prog_char scope_menu_1[] PROGMEM = "Pan Man. Spd.";
prog_char scope_menu_2[] PROGMEM = "Tilt Man. Spd.";

 // menu organization

PROGMEM const char *menu_str[]  = { menu_1, menu_2, menu_3, menu_4, menu_5, menu_6 };

PROGMEM const char *man_str[]   = { manual_menu_1, manual_menu_2, manual_menu_3 };

PROGMEM const char *axis0_str[] = { axis_menu_1, axis_menu_10, axis_menu_11, axis_menu_2, axis_menu_3, axis_menu_4, axis_menu_5, axis_menu_12, axis_menu_6, axis_menu_7, axis_menu_8, axis_menu_9 };
PROGMEM const char *axis1_str[] = { axis_menu_1, axis_menu_10, axis_menu_11, axis_menu_2, axis_menu_3, axis_menu_4, axis_menu_5, axis_menu_12, axis_menu_6, axis_menu_7, axis_menu_8, axis_menu_9 };
PROGMEM const char *cam_str[]   = { camera_menu_1, camera_menu_2, camera_menu_3, camera_menu_4, camera_menu_5, camera_menu_6, camera_menu_7, camera_menu_8 };
PROGMEM const char *set_str[]   = { set_menu_1, set_menu_2, set_menu_3, set_menu_4, set_menu_5, set_menu_6, set_menu_7, set_menu_8, set_menu_9, set_menu_10, set_menu_11, set_menu_12, set_menu_13, set_menu_14 };
PROGMEM const char *scope_str[] = { scope_menu_1, scope_menu_2 };

 // max number of inputs for each menu (in order listed above, starting w/ 0)

 byte max_menu[7]  = {5,2,10,10,7,13,1};

 // support a history of menus visited up to 5 levels deep
byte hist_menu[5] = {0,0,0,0,0};

char lcd_buf[MAX_LCD_STR];

  // what is our currently selected menu?
  // what is our current position?
byte cur_menu      = 0;
byte cur_pos       = 0;
byte cur_pos_sel   = 0;

  // which input value position are we in?
byte cur_inp_pos   = 0;

  // input buffers
unsigned long cur_inp_long  = 0;
float cur_inp_float         = 0.0;
boolean cur_inp_bool        = false;

  // which input are we on, if on
  // the main screen.
  
byte main_scr_input         = 0;

  // last read button (analog) value
int last_but_rd = 1013;

  // flags for each button
  // use indivial bits to indicate
  // whether a given button was pressed.
  
byte button_pressed = 0;

  // input value multiplier
  
byte inp_val_mult = 1;

  // how long has a button been held for?
unsigned long hold_but_tm = 0;
  // when was ui last updated on home scr?
unsigned long ui_update_tm = 0;

  // lcd dim control
byte cur_bkl     = 255;
boolean blank_lcd   = false;

 // for dimming lcd
unsigned int lcd_dim_tm     = 5;
unsigned long input_last_tm = 0;

 // show cm instead of inch?
boolean ui_is_metric = false;
  // floats are input in tenths?
boolean ui_float_tenths = false;

 /* user interface control flags
 
   B0 = update display
   B1 = currently in setup menu
   B2 = in value entry
   B3 = have drawn initial value in value entry
   B4 = have used decimal in current value
   B5 = in manual mode
   B6 = lcd bkl on
   B7 = in calibrate mode
   
 */
   
byte ui_ctrl_flags = B00000000;

 /* calibration screen flags
 
    B0 = Currently calibrating
    B1 = Done Calibrating
    
 */
 
byte ui_cal_scrn_flags = 0;

 // whether to show ipm (true) or pct (false)
 
boolean ui_motor_display = true;

 /* input type flags
 
   B0 = input value is a float
   B1 = input is a bool (on/off) value
   B2 = input is a bool (up/dn) value
   B3 = input is a bool (lt/rt) value
   B4 = input is a bool (ipm/pct) value
   B5 = input is a bool (pulse/sms) value
   B6 = input is a bool (rotary/linear) value
   B7 = input is list (0,45,90) value
   
 */
 
byte ui_type_flags = 0;

 /* input type flags vector #2
 
   B0 = input value is list (Disable/Start/Stop)
   B1 = input value is forced metric
   
 */
 
byte ui_type_flags2 = 0;



  /* run status flags
  
    B0 = running
    B1 = camera currently engaged
    B2 = camera cycle complete
    B3 = motors currently running
    B4 = external trigger engaged
    
  */
  
volatile byte run_status = 0;

  /* external intervalometer
    
    B0 = I/O 1 is external intervalometer
    B1 = I/O 2 is external intervalometer
    B2 = interval OK to fire
  
  */
  
byte external_interval = 0;

 /* external trigger via alt i/o pins
 
  B0 = I/O 1 external enabled
  B1 = I/O 2 external enabled

 */
  
byte external_trigger  = 0;

 // trigger delays
unsigned long ext_trig_pre_delay = 0;
unsigned long ext_trig_pst_delay = 0;

 // motor slow mode is pulse (true) or sms (false)
boolean motor_sl_mod = true;

 // camera exposure time
unsigned int exp_tm      = 100;
 // tap focus before exposing
unsigned int focus_tap_tm = 0;
 // delay after exposing (mS)
unsigned int post_delay_tm      = 100;
 // brign focus pin high w/ shutter
boolean focus_shutter   = true;
 // intervalometer time (seconds)
float cam_interval = 1.0;
 // max shots
unsigned int cam_max  = 0;
  // camera repeat shots
byte cam_repeat = 0;
  // delay between camera repeat cycles
unsigned int cam_rpt_dly = 250;

byte pre_focus_clear      = 0;
unsigned long cam_last_tm = 0;

  // currently selected motor
byte cur_motor = 0;
  // set speed for the current motor
unsigned int m_speeds[2] = {0,0};
  // currently set speed (for altering motor speed)
unsigned int mcur_spds[2] = {0,0};
  // prev direction for motor
byte m_wasdir[2] = {0,0};
  // distance (i) per revolution
float m_diarev[2] = {3.53, 3.53};
  // motor RPMs
float m_rpm[2]    = { 8.75, 8.75 };
  // calculated max ipm
float max_ipm[2] = {m_diarev[0] * m_rpm[0], m_diarev[1] * m_rpm[1]};
  // user-configurable min ipm
float min_ipm[2] = {20.0, 20.0};
  // minimumspeed (min ipm->255 scale value)
byte min_spd[2] = { (min_ipm[0] / max_ipm[0]) * 255, (min_ipm[1] / max_ipm[1]) * 255 };
 // minimum pulse cycles per motor
byte m_min_pulse[2] = { 125, 125 };
 // calibration points
byte motor_spd_cal[2] = {1,40};

  // linear or rotation type?
byte m_type[2] = {0,0};
  // fixed sms?
byte m_smsfx[2] = {0,0};
  // maximum sms distance
unsigned int m_maxsms[2] = { max_ipm[0] * 100, max_ipm[1] * 100};


 // for timer1 pulsing mode control
boolean timer_used = false;
volatile  bool timer_engaged      = false;
volatile bool motor_engaged      = false;
volatile byte motor_ran = 0;

 // motor calibration

float m_cal_constant[2] = {0.69, 0.69};

float m_cal_array[2][3][3][2] = { 
              { 
                {
                  {0.61914329,0.61914329},{1.0,1.0},{2.01133251,2.11453032}
                },
                {
                  {0.61914329,0.61914329},{1.0,1.0},{2.01133251,2.11453032}
                },
                {
                  {0.61914329,0.61914329},{1.0,1.0},{2.01133251,2.11453032}
                } 
              }, 
              { 
                {
                  {0.61914329,0.61914329},{1.0,1.0},{2.01133251,2.11453032}
                },
                {
                  {0.61914329,0.61914329},{1.0,1.0},{2.01133251,2.11453032}
                },
                {
                  {0.61914329,0.61914329},{1.0,1.0},{2.01133251,2.11453032}
                } 
              } 
            };
            
byte m_cur_cal = 0;
byte m_angle[2] = {0,0};

boolean m_cal_done = false;

 // ramping data
byte m_ramp_set[2]     = {0,0};
float m_ramp_shift[2]  = {0.0,0.0};
byte m_ramp_mod[2]     = {0,0};

 // lead-ins for axis movement
unsigned int m_lead_in[2] = {0,0};
unsigned int m_lead_out[2] = {0,0};

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
 
byte input_type[2]            = {0,0};
unsigned long input_trig_last = 0;


boolean merlin_enabled = false;

byte  merlin_dir[2]        = {0,0};
byte  merlin_wasdir[2]     = {0,0};
float merlin_speeds[2]     = {0.0,0.0};
float merlin_man_spd[2]    = {1440.0,1440.0};

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
 delay(1000);

    // check firmware version stored in eeprom
    // will cause eeprom_saved() to return false
    // if version stored in eeprom does not match
    // firmware version.  This automatically clears
    // saved memory after a new firmware load -
    // saving lots of support questions =)
    
 eeprom_versioning();
 
   // did we previously save settings to eeprom?
 if( eeprom_saved() ) {
     // restore saved memory
   restore_eeprom_memory();
 }
 else {
     // when memory has been cleared, or nothing has been
     // saved, make sure eeprom contains default values
   write_all_eeprom_memory();
 }

 show_home();
 
 /*
 for( byte i = 0; i <= 2; i++) {
   Serial.print(i, DEC);
   Serial.print(":");
   for ( byte x = 0; x < 2; x++ ) {
     Serial.print(m_cal_array[0][0][i][x], 8);
     Serial.print(":");
   }
   Serial.println("");
 } 
 */
 
}



void loop() {

  if( run_status & B10000000 ) {
    // program is running
    main_loop_handler();
  } // end if running
  else {
      // not running, not in manual, but merlin motors are running?
    if( ! ( ui_ctrl_flags & B00000100 ) && merlin_flags & B11000000 ) {
      merlin_stop(0);
      merlin_stop(1);
    }
  }
  
    // always check the UI for input or
    // updates
    
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
  
  if( cam_max > 0 && shots >= cam_max && ( ok_stop || (m_speeds[0] <= 0.0 && m_speeds[1] <= 0.0) || motor_sl_mod ) ) {
    
     // stop program if max shots exceeded, and complete cycle completed
     // if in interleave, ignore ocmplete cycle if in pulse
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
  else  if( motor_sl_mod &&
            (( m_speeds[0] > 0 && m_speeds[0] < min_spd[0] ) ||
             ( m_speeds[1] > 0 && m_speeds[1] < min_spd[1] ) ) ) {
        
      // if pulse mode is on and at least
      // one motor needs to be pulsed...
    
    motor_run_pulsing();
        
  }
  
    // run merlin in continuous mode if needed
    
  if( motor_sl_mod && merlin_enabled ) {
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
  else if( run_status & B01001000 || pre_focus_clear == 3 ) {
      // currently firing the camera, focus, or triggering an external
      // control line
      
      // do nothing
    ;
  }
  else if( run_status & B00100000 ) {
      // camera cycle completed
        // clear exposure cycle complete flag
    run_status &= B11011111;
    if( camera_fired == true ) {
      // the shot just fired
      camera_fired = false;
      shots++;
      
      
        // for ramping motor speeds
        // we change speed in ramps after shots...

      motor_execute_ramp_changes();
        // check to see if a post-exposure delay is needed
      
      if( post_delay_tm > 0 ) {
          // we block anything from happening while in the
          // post-exposure cycle by pretending to be an
          // exposure
        run_status |= B01000000;
        
        MsTimer2::set(post_delay_tm, camera_clear);
        MsTimer2::start();
        
        motors_clear = false;
        ok_stop = false;
      }
      else {
          // no post-exp delay, is the external trigger to fire?
        if( external_trigger & B11000000 && ext_trig_pst_delay > 0 )
          alt_ext_trigger_engage(false);
       

        //no post-exposure delay, motors can run
        motors_clear = true;
      }
        
    } 
    else {
        // this was a post-exposure delay cycle completing, not
        // an actual shot
        
        // is the external trigger to fire?
      if( external_trigger & B11000000 && ext_trig_pst_delay > 0 )
        alt_ext_trigger_engage(false);
      

        // we can set the motors clear to move now
      motors_clear = true;        
    }
    
        // is the merlin head set to move?    
    if( motors_clear == true && merlin_enabled  && ! motor_sl_mod ) {
        // send merlin head to move sms distances (if desired)             
  
      if( merlin_speeds[0] > 0 ) {        
        merlin_send_angle(0, merlin_speeds[0]);
        ok_stop = false;
      }
      if( merlin_speeds[1] > 0 ) {
        merlin_send_angle(1, merlin_speeds[1]);
        ok_stop = false;
      }
      
    }

  }
  else if( motors_clear == true && ! motor_sl_mod && 
            ( merlin_enabled && ( merlin_flags & B11000000 ) ) ) {
    
        // merlin motors are currently running!
        // other actions cannot continue...
        
    if( ! merlin.readAxisStatus(1) && ! merlin.readAxisStatus(2) ) {
        // but not actually running... now remove the flags
        // indicating so.
      merlin_flags &= B00111111;
      ok_stop = true;
    }  
    
  } 
  else if( motors_clear == true && ! motor_sl_mod &&
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
  else if( external_interval & B11000000 ) {
    // external intervalometer is engaged
    
    if( external_interval & B00100000 ) {
      // external intervalometer has triggered
 
          // clear out ok to fire flag
      external_interval &= B11011111;      
      do_fire = true;
    }
  }
  else if( cam_last_tm < millis() - (cam_interval * 1000) ) {
      // internal intervalometer triggers
    do_fire = true;
  }
  
  if( do_fire == true ) {
      // we've had a fire camera event
    
           // is the external trigger to fire?
    if( external_trigger & B11000000 && ext_trig_pre_delay > 0 && ext_trip == false && (cam_repeat == 0 || cam_repeated == 0) ) {
        alt_ext_trigger_engage(true);
        ext_trip = true;
    }
    else {

        // make sure we handle pre-focus tap timing
        
      if( ( pre_focus_clear == 4 || focus_tap_tm == 0 || (cam_repeat > 0 && cam_repeated > 0) ) && !(run_status & B00001000) ) {
  
          // we always set the start mark at the time of the
          // first exposure in a repeat cycle (or the time of exp
          // if no repeat cycle is in play
          
        if( cam_repeat == 0 || cam_repeated == 0 )
          cam_last_tm  = millis();
  
          // deal with camera repeat actions
        if( cam_repeat == 0 || (cam_repeat > 0  && cam_repeated >= cam_repeat) ) {
          camera_fired = true;
          do_fire = false;
          ext_trip = false;
          cam_repeated = 0;
        }
        else if( cam_repeat > 0 ) {
            // only delay ater the first shot
          if( cam_repeated > 0 )
            delay(cam_rpt_dly); // blocking delay between camera firings (we should fix this later!)
            
          cam_repeated++;
        }
        
        // camera is all clear to fire, and enough
        // time is elapsed
        fire_camera(exp_tm);
        pre_focus_clear = 0;
        
      }
      else if( focus_tap_tm > 0 && pre_focus_clear == 0 && !(run_status & B00001000) ) {
          // pre-focus tap is set, bring focus line high
        digitalWrite(FOCUS_PIN, HIGH);
        MsTimer2::set(focus_tap_tm, stop_cam_focus);
        MsTimer2::start();
        pre_focus_clear = 1;
      }
    } // end else (not external trigger...
  } // end if(do_fire...
}


void start_executing() {
  // starts program execution
  
   run_status |= B10010000;
  
    // turn on motors
  motor_control(0,true);
  motor_control(1,true);
  
    // if ramping is enabled for a motor, start at a zero
    // speed
  if( m_ramp_set[0] >= 1 )
      motor_set_speed(0, 0); 
  if( m_ramp_set[1] >= 1 )
      motor_set_speed(1, 0); 

    // reset shot counter
  shots = 0;
}

void stop_executing() {
  run_status &= B01100111;
  motor_stop_all();
}

  
