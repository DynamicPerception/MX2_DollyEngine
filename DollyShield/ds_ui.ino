/* 

   "DollyShield" MX2
   
   (c) 2010 C.A. Church / Dynamic Perception LLC
   (c) 2014 William B Phelps
   
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

  ========================================
  Core UI functions
  ========================================
  
  WBP TODO:
  - button debounce
  - manual control motor speed separate vars
  - menu mode: 0=original, 1=new
  - underline cursor
  
*/

// #define DBG

static bool Cursor_Enabled;
static bool Cursor_On;
void setBlink(bool set)
{
  Cursor_Enabled = set;
	if (set) {
		if (EE.ul_cursor) 
		  lcd.cursor(); // underline cursor
		else
		  lcd.blink(); // hardware block cursor
	  Cursor_On = true;
	}
	else {
		if (EE.ul_cursor) 
		  lcd.noCursor(); // underline
		else
		  lcd.noBlink(); // hardware block cursor
	}
}  

void init_user_interface() {

  pinMode(LCD_BKL, OUTPUT);
  
    // turn on lcd backlight
  analogWrite(LCD_BKL, 255);

    // init lcd to 16x2 display
  lcd.begin(16, 2);
  lcd.setCursor(0,0);

    // clear and turn on autoscroll
 lcd.clear();
 //lcd.autoscroll();
 
   // banner
   
 lcd.print("(c) 2014 Dynamic");
 lcd.setCursor(5,1);
 lcd.print("Perception");

 delay(2000);

 lcd.clear(); 

 lcd.setCursor(0,0); 
 lcd.print("MX2 Dolly Engine");
 lcd.setCursor(0,1);
 lcd.print("  Version 0.94W ");
 
   // setup button input

 pinMode(BUT_PIN, INPUT);   
   // enable internal pull-up
 digitalWrite(BUT_PIN, HIGH);
 
 
   // set the update screen flag (draw main screen)
 ui_ctrl_flags |= UC_Update;
 ui_ctrl_flags |= UC_Backlight_On;
 
 delay(2000);
 
}

	// debounces both on & off states
bool get_switch(byte which) {
	bool ext = digitalRead(EXT_PIN1 + which) == LOW; // check switch state - LOW is closed
	if (ext != ext_trigger_last[which]) { // did it change?
		ext_trigger_last[which] = ext; // remember new state
		ext_trigger_tm[which] = millis(); // remember when it happened
	}
		// switch state did not change since last call, has it settled?
	else if ((millis() - ext_trigger_tm[which] >= TRIGGER_DEBOUNCE_MS)) {
		ext_trigger_state[which] = ext; // change the switch state
	}
	return ext_trigger_state[which];
}

void check_switch(byte which) {
	bool sw = get_switch(which); // get current switch state
	if (sw != input_trigger[which]) { // has it changed? 
		input_trigger[which] = sw; // remember new state
		if (input_trigger[which]) { // is switch on (closed)?
			if (run_status & RS_Parking) {
				motor_control(0, false); // stop motor
				run_status &= (255-RS_Parking); // not parking now
				cur_inp_long = 0; // show "Stopped"
				draw_values(park_str, true, true); // update display
			}
			else {
				byte code = EE.input_type[which];
				switch (code) {
					case IT_Start:
						start_executing();
						break;
					case IT_Stop:
						stop_executing();
						break;
					case IT_Toggle:
						altio_flip_runstat();
						break;
					case IT_ExtInterval:
						external_interval |= B00100000;  // set camera to fire
						break;
					case IT_ChangeDir:
						// switch all motor directions!
						motor_dir(0, !EE.m_dirs[m_mode][0]);
						motor_dir(1, !EE.m_dirs[m_mode][1]);
						break;
				}
			}
		}
	}
}


void check_user_interface() {
  
    // check for external interrupt requests
  check_switch(0);
  check_switch(1);
    
    // turn off/on lcd backlight if needed
  if( (ui_ctrl_flags & UC_Backlight_On) && input_last_tm < millis() - (EE.lcd_dim_tm * 1000) ) {
    ui_ctrl_flags &= (255-UC_Backlight_On);
    
    if( EE.blank_lcd ) 
      lcd.noDisplay();
      
    digitalWrite(LCD_BKL, LOW);
  }
  else if( ! (ui_ctrl_flags & UC_Backlight_On) && input_last_tm > millis() - (EE.lcd_dim_tm * 1000) ) {
    ui_ctrl_flags |= UC_Backlight_On;
    
    lcd.display();
    
    analogWrite(LCD_BKL, EE.cur_bkl);
  }
  
  if ( EE.ul_cursor && Cursor_Enabled && (ui_blink_tm < millis() - 500)) {
    if (Cursor_On) {
      lcd.noCursor();
      Cursor_On = false;
    }
    else {
      lcd.cursor();
      Cursor_On = true;
    }
    ui_blink_tm = millis();
  }
  
    // if we're set to update the display
    // (on-demand or once a second when not
    // in a menu)
  if( ui_ctrl_flags & UC_Update ||
      ( (ui_update_tm < millis() - 1000) && ! (ui_ctrl_flags & B01000000) ) ) {
  
      // determine whether to show home or manual screen      
    if( ! ( ui_ctrl_flags & UC_Manual ) ) {
      show_home();
    }
    
//#ifdef MERLIN_ENABLED
    else if( merlin_flags & B00100000 ) {
      show_merlin();
    }
//#endif

    else {
      show_manual();
    }
     
    ui_ctrl_flags &= (255-UC_Update);
    ui_update_tm = millis();
  }
  
    // make sure to turn off motor if in manual control and no button is held
  byte held = ui_button_check();
  if( ui_ctrl_flags & UC_Manual && held == false && run_status & RS_Motors_Running ) {
    motor_control(cur_motor, false); // stop motor
    show_manual(); // update screen
  }

}

byte ui_button_check() {

  static byte hold_but_cnt = 0;
  boolean held = false;
  
  byte button = get_button(); // any button down now?
 
  byte bt_press = check_button( button); // first press, hold, repeating?
   
  if( bt_press == 0 ) 
    return(false);  // not pressed
   
  if(  bt_press == 1 ) { // button is pressed (first time)
    hold_but_cnt  = 0;
    inp_val_mult  = 1;
    handle_input(button, false);
  }
  else if( bt_press >= 2 ) {
    held = true;
    
    if ( bt_press == 3) {  // button repeating?
      handle_input(button, true);
      hold_but_cnt++;
      if( hold_but_cnt >= 10 ) {
        if (inp_val_mult < 1000)
          inp_val_mult *= HOLD_BUT_VALINC;
        else
          inp_val_mult = 1000;
        hold_but_cnt = 0;
      }
		}

  } // end else if button press state == 2

  return(held);

}

byte get_button() {
  // see which button is pressed (only supports one button at a time)
  // buttons are on one analog pin, the value
  // determines which button, if any is pressed
	byte bp = 0;

  // read analog input
  int val_read = analogRead(BUT_PIN - 14);

    // don't let it flip in a single read
		// did value change by more than the threshold?
  if( abs(last_but_rd - val_read) > BUT_THRESH ) { 
    last_but_rd = val_read;
    but_push_tm = millis(); // start debounce timer
    return 0;
  }

		// value is consistent, how long has it been this way?
	if (millis() - but_push_tm < BUT_DEBOUNCE_MS) {
		return 0; // not long enough yet
	}
  
  if( val_read > (BUT0_VAL - BUT_THRESH) && val_read < (BUT0_VAL + BUT_THRESH) ) {
    bp = BUT0;
  }
  else if( val_read > (BUT1_VAL - BUT_THRESH) && val_read < (BUT1_VAL + BUT_THRESH) ) {
    bp = BUT1;
  }
  else if( val_read > (BUT2_VAL - BUT_THRESH) && val_read < (BUT2_VAL + BUT_THRESH) ) {
    bp = BUT2;
  }
  else if( val_read > (BUT3_VAL - BUT_THRESH) && val_read < (BUT3_VAL + BUT_THRESH) ) {
    bp = BUT3;
  }
  else if( val_read > (BUT4_VAL - BUT_THRESH) && val_read < (BUT4_VAL + BUT_THRESH) ) {
    bp = BUT4;
  }
  
  return bp;

}

unsigned long button_down_tm = 0;
byte button_pressed = 0;
unsigned long button_hold_ms = BUT_HOLD_MS;

  // this handles only one button at a time
  // returns 0 for not pressed, 1 for first press, 2 for held
byte check_button(byte button) {
  
    // determine if the given button was
    // pressed, held, or is neither

    // if this button was pressed at last call
  if( button>0 && button_pressed == button ) {
    if ((millis() - input_last_tm) > button_hold_ms) {  // has it been held long enough?
	
			if ((millis() - button_down_tm) > button_hold_ms) {  // 
				button_down_tm = millis();
				button_hold_ms = BUT_REPEAT_MS;
				return(3); // button held down & repeating
			}
			else
				return(2); // just held

		}
		else
      return(0); // not first time, not held yet, so return 0
  }
    
  button_pressed = button; // record new (or no) button down
  if (button > 0) {
    input_last_tm = millis();  // record when any button was pushed
    button_down_tm = millis(); // record when it happened  
    return(1); // first press
		}
  else {
    button_hold_ms = BUT_HOLD_MS; // reset button repeat timer
    return(0); // no button down
	}
}
  
  
byte get_menu( byte mnu, byte pos ) {

  // where is our target menu when 
  // mnu.pos is pressed?

  switch(mnu) {
    case MU_MAIN:
      if( pos <= max_menu[0] ) // are there more?
        return( pos + 1 );
      break;
    
    case MU_MANUAL:
        // manual control is special return code
      return(MU_SPECIAL);

//    default:
//      break;        
  }

  // default is 'no target', an input value
  return(MU_NOTARGET);
 
}

/*
   handle user input
*/

void handle_input( byte button, boolean held ) {
  
  // do what needs to be done when whatever
  // button is hit
  
  if( button == BUT_CT ) {  // call center button function
    ui_button_center(held);    
  }
  
  else if( button == BUT_DN ) {
    ui_button_down(held);
  }
  
  else if( button == BUT_UP ) {
    ui_button_up(held);
  }
  
  else if( button == BUT_RT ) {
    ui_button_rt(held);
  }  
  else if( button == BUT_LT ) {
    ui_button_lt(held);
  }

  return;

}


  // button handlers
  
  
void menu_select() { 	// in a setup menu, find the next menu to go to
		
			// calibration, don't do anything else
			// EJD:20130329: Corregido && cur_pos == 6 que no dejaba acceder a calibrar la constante 
		if( (cur_menu == MU_AXIS1 || cur_menu == MU_AXIS2) && cur_pos == 5 ) {  // Axis1 or Axis2, and Calibrate
			get_value(cur_menu, cur_pos, false);
			return;
		}

		byte new_menu = get_menu(cur_menu, cur_pos);
			
			// if drawing motor manual screen...
		if( new_menu == MU_SPECIAL ) {
			get_value(cur_menu, cur_pos, false);
			return;
		}
		
		if( new_menu == MU_NOTARGET ) {
			if ( ! (ui_ctrl_flags & UC_Value_Entry) )
					// this is not a new menu, but it is a new input of some sort
				draw_menu(3,true);
			return;
		}

			// entering another menu
			
			// record the last menu we were at
		push_menu(cur_menu, cur_pos);

			// clear in value setting flag
		ui_ctrl_flags &= (255-UC_Value_Entry);

			// set menu to new menu
		cur_menu = new_menu;
		draw_menu(0,false);

}  
  
  
void ui_button_center( boolean held ) {
      // center button

        // on calibration screen ?
     if( ui_ctrl_flags & UC_Calibrate ) {
       
       if( ui_cal_scrn_flags & US_CalibrateDone ) {
         // completed calibrating
         ui_cal_scrn_flags &= (255-US_Calibrate-US_CalibrateDone);
         show_calibrate();
         return;
       }  
       else if( ui_cal_scrn_flags & US_Calibrate ) {
         // in calibrating input
         
         if( held == true )
           return;
           
         m_cal_done = true;
         return;
       }
       
       execute_calibrate();
       return;
     }


     if( main_scr_input > 0 ) {
         // make sure to abort main screen input
       setBlink(false);
       main_scr_input = 0;
     }
     
     
       // if in manual control
     if( ui_ctrl_flags & UC_Manual  ) {
       
       EE.m_speeds[1][0] = m_speeds[1][0]; // copy current motor speeds back to EE vars
       EE.m_speeds[1][1] = m_speeds[1][1];
       ee_save(); // save
         
         // clear out manual ctrl flag
       ui_ctrl_flags &= (255-UC_Manual);
       m_mode = 0; // set motor speed index to normal
       
       merlin_flags &= B11011111;

         // resume back to setup menu
       ui_ctrl_flags |= UC_Setup;
       cur_menu = MU_MANUAL; // show menu again
       draw_menu(0, false);
       return;
     }

    if( ! (ui_ctrl_flags & UC_Setup) ) {
        // not in any setup menu
      ui_ctrl_flags |= UC_Setup;
      cur_menu = MU_MAIN;
      draw_menu(0,false);
      return;
    }
			
		if( ui_ctrl_flags & UC_Value_Entry ) {
				// exiting out of value entry (save...)
				// go back to current menu

				// set/save new value 
			get_value(cur_menu, cur_pos, true);
			
				// reset the float tenths (vs 100ths) parameter
			ui_float_tenths = false;
			
				// clear in value setting flag
				// and the flag indicating that
				// we've already displayed this value
			if (ui_type == UT_Park) { // enter key does not exit park screen
//				ui_ctrl_flags &= (255-UC_Value_Drawn); // display value again
				draw_menu(3,true);
				}
			else {
				ui_ctrl_flags &= (255-UC_Value_Entry-UC_Value_Drawn);
				draw_menu(3,false);
			}
		}

//		if (!EE.alt_menu)  // original menu style?
		  menu_select();  // enter key selects menu items

}


void ui_button_down( boolean held ) {

        // on calibration screen

     if( ui_ctrl_flags & UC_Calibrate ) {
         
       if( ui_cal_scrn_flags & US_Calibrate ) {
         // in calibrating settings
         move_val(false);
         update_cal_screen();
         return;
       }
         
       m_cur_cal = m_cur_cal > 0 ? m_cur_cal - 1 : 0;
       show_calibrate();
       
       return;
     }
  

      // if in manual motor mode...
    if( ui_ctrl_flags & UC_Manual ) {

      if( merlin_flags & B00100000 ) {

        if( held == true ) 
           return;
        
        if( merlin_flags & B01000000 ) {
            merlin_stop(1);
              // speed was modified by running, return to
              // original value
            merlin_set_speed(1, merlin_speeds[1]);
          }
          else {
            // AC:20120114: moved toe code to merlin_move_manual() to save a few bytes
            merlin_move_manual(1,1);
          }

        show_merlin();
      }
      else {
        motor_speed_adjust(cur_motor, -1 * inp_val_mult, true);
        show_manual();
      }

      return;
    }
    
      // if not currently in setup menus, or
      // modifying a main screen value
    if( ! (ui_ctrl_flags & UC_Setup) & main_scr_input == 0 ) {
        // alternate between merlin and home screens
      if( EE.merlin_enabled ) {
          // switch between merlin and normal home screens
        if( merlin_flags & B00010000 ) {
          merlin_flags &= B11101111;
        }
        else {          
          merlin_flags |= B00010000;
        }
      return;
      }
    return; // dead spot
    }

   if( main_scr_input > 0 ) {
     if ( (main_scr_input > 1) || (run_status & RS_Running))  // don't start running with down button (wbp)
       move_val(false);
       // save present value
     get_mainscr_set(main_scr_input, true);
       // set screen to update
     ui_ctrl_flags |= UC_Update;
   }
   else if( ui_ctrl_flags & UC_Value_Entry ) {
        // entering a value
      move_val(false);
      draw_menu(3,true);
    }
   else {
       // moving to next menu item
     draw_menu(2,false);
   }
    
}

void ui_button_up( boolean held ) {

        // on calibration screen
     if( ui_ctrl_flags & UC_Calibrate ) {
         
       if( ui_cal_scrn_flags & US_Calibrate ) {
         // in calibrating settings
         move_val(true);
         update_cal_screen();
         return;
       }

       m_cur_cal = m_cur_cal > 1 ? 2 : m_cur_cal + 1;
       show_calibrate();
       
       return;
     }
  
      // if in manual motor mode...
    if( ui_ctrl_flags & UC_Manual ) {

      if( merlin_flags & B00100000 ) {

          if( held == true ) 
           return;

        
          if( merlin_flags & B01000000 ) {
            merlin_stop(1);
              // speed was modified by running, return to
              // original value
            merlin_set_speed(1, merlin_speeds[1]);
          }
          else {
            // AC:20120114: changed to merlin_move_manual()
            merlin_move_manual(1,0);
          }

        show_merlin();
      }
      else {
        motor_speed_adjust(cur_motor, 1 * inp_val_mult, true); // wbp 
        show_manual();
      }
      return;
    }
    
    if( ! (ui_ctrl_flags & UC_Setup) & main_scr_input == 0 ) {
        // alternate between merlin and home screens
      if( EE.merlin_enabled ) {
          // switch between merlin and normal home screens
        if( merlin_flags & B00010000 ) {
          merlin_flags &= B11101111;
        }
        else {          
          merlin_flags |= B00010000;
        }
      return;
      }
    return; // dead spot, no input
    }
      
   if( main_scr_input > 0 ) {
    
     move_val(true);
       // save present value
     get_mainscr_set(main_scr_input, true);
     
       // set screen to update
     ui_ctrl_flags |= UC_Update;
   }
   else if( ui_ctrl_flags & UC_Value_Entry ) {
        // entering a value
      move_val(true);
      draw_menu(3,true);
    }
   else {
     draw_menu(1,false);
   }

}


void menu_back() {  // go back one level in menu
     // clear out calibration screen value, if on
    if( ui_ctrl_flags & UC_Calibrate )
      ui_ctrl_flags &= (255-UC_Calibrate);
      
    if (ui_ctrl_flags & UC_Park) {
			if (run_status & RS_Parking) {
				return; // if parking, can't leave the park screen
			}
			m_mode = 0; // motor speed index to normal
			ui_ctrl_flags &= (255-UC_Park);
		  altio_connect(0, EE.input_type[0]); // reset input switch 
    }
    
    if( ui_ctrl_flags & UC_Value_Entry ) {
        // we're in a value entry mode.  Exit
        // entry without saving the value

        // clear in value setting flag
        // and the flag indicating that
        // we've already displayed this value
      ui_ctrl_flags &= (255-UC_Value_Entry-UC_Value_Drawn);
              // reset the float tenths (vs 100ths) parameter
      ui_float_tenths = false;

//      draw_menu(0,false);
      draw_menu(3,false); // redraw menu
      return;
    }
    
      // draw previous menu
      
    if( cur_menu == MU_MAIN ) { 
        // we're at the highest menu, back to main screen 
      cur_pos = 0;  
        // clear setup flag
        // indicate display needs updating
      ui_ctrl_flags &= (255-UC_Setup);
      ui_ctrl_flags |= UC_Update;
        // clear out list of menus
      flush_menu(); // not really necessary now but it can't hurt...
      main_scr_input = 1; // reset to on/off position
      setBlink(true);
    }
    else {
        // a parent menu can be drawn
      pop_menu(cur_menu, cur_pos);
      draw_menu(3,false);
    }
}


void ui_button_rt( boolean held ) {
  
      // if in manual control
    if( ui_ctrl_flags & UC_Manual ) {

      if( merlin_flags & B00100000 ) {

         if( held == true ) 
           return;
           
         if( merlin_flags & B10000000 ) {
           merlin_stop(0);
              // speed was modified by running, return to
              // original value
            merlin_set_speed(0, merlin_speeds[0]);
          }
         else {
             // AC:20120114: changed to merlin_move_manual()
             merlin_move_manual(0,0);
         }
          
        //show_merlin();
      }
      else {  // in manual
        if( held == true ) {
          if( ! (run_status & RS_Motors_Running) ) {
            // change or set motor direction
            motor_dir(cur_motor, 0);
            // start motor
            motor_control(cur_motor, true);
            }
          }
        show_manual();
      }
      
      return;
    }  


    if( ! (ui_ctrl_flags & UC_Setup) ) {
      // we're on main screen, rt switches value we can
      // adjust
      main_screen_select(true);
      return;
    }
    
    if (EE.alt_menu)
      menu_select(); // right button selects menu item
    else
      menu_back(); // go back to (previous) menu

}


void ui_button_lt(boolean held) {
        // if in manual control
    if( ui_ctrl_flags & UC_Manual ) {
      
      if( merlin_flags & B00100000 ) {

          if( held == true ) 
           return;

          if( merlin_flags & B10000000 ) {
            merlin_stop(0);
              // speed was modified by running, return to
              // original value
            merlin_set_speed(0, merlin_speeds[0]);
          }
          else {
            // AC:20120114: changed to merlin_move_manual()              
            merlin_move_manual(0,1);  
          }

      }
      else {
        if( held == true ) {        
          if( ! (run_status & RS_Motors_Running) ) {
            // set or change motor direction
            motor_dir(cur_motor, 1);
            // get motor moving
            motor_control(cur_motor, true);
          }
        }

        show_manual();
      }

      return;
    }  

    if( ! (ui_ctrl_flags & UC_Setup) ) {
      // we're on main screen, lt switches value we can
      // adjust
      main_screen_select(false);
      return;
    }

		if (EE.alt_menu)
      menu_back(); // left button goes back one level 

}



/* 
  Draw screens
*/


void draw_menu(byte dir, boolean value_only) {
  
    // turn off blinking, if on...
  setBlink(false);
  
  boolean draw_all = false;

  // determine the direction we are going, up/down (1/2),
  // draw all (but don't move position) (3), and draw
  // new menu from top (0)
  
  if( dir == 1 ) {
      // up
    cur_pos = cur_pos == 0 ? max_menu[cur_menu] : cur_pos - 1;
    if( cur_pos < cur_pos_sel || cur_pos == max_menu[cur_menu]) {
      lcd.clear();
      draw_all = true;
    }
  }
	else if( dir == 2 ) {
      // down
    cur_pos = cur_pos < max_menu[cur_menu] ? cur_pos + 1 : 0 ;
    if( cur_pos > cur_pos_sel || cur_pos == 0) {
      lcd.clear();
      draw_all = true;
    }
  }    
  else if( dir == 3 ) {
      // draw all (no move)
    draw_all = true;  // really? (wbp)
  }
  else {
      // draw new menu (from top)
    cur_pos = 0;
    draw_all = true;
  }
  
  switch( cur_menu ) {
    
    case MU_MAIN:
    
      draw_values(menu_str, draw_all, value_only);
      break;
      
    case MU_MANUAL:
    
      draw_values(man_str, draw_all, value_only);
      break;
      
    case MU_AXIS1:
    
      draw_values(axis0_str, draw_all, value_only);
      break;
      
    case MU_AXIS2:
    
      draw_values(axis1_str, draw_all, value_only);
      break;

    case MU_CAMERA:

      draw_values(cam_str, draw_all, value_only);
      break;

    case MU_PARK:

      draw_values(park_str, draw_all, value_only);
      break;
    
    case MU_RESET:

      draw_values(reset_str, draw_all, value_only);
      break;
  
    case MU_SETTINGS:
    
      draw_values(set_str, draw_all, value_only);
      break;

    case MU_SCOPE:
    
      draw_values(scope_str, draw_all, value_only);
      break;
      
    default: 
      return;  
  }
  

}

   
void draw_values(const char *these[], boolean draw_all, boolean value_only) {
  
  if( draw_all == true ) {

    // must draw the whole display
    lcd.clear();
      // clear out lcd buffer
      
    memset(lcd_buf, ' ', sizeof(char) * MAX_LCD_STR);
    
      // draw first option
    lcd.noCursor();
    lcd.setCursor(0,0);
    cur_pos_sel = cur_pos;
    
    strcpy_P(lcd_buf, (char*) pgm_read_word(&(these[cur_pos])));
    lcd.print("> ");
    lcd.print(lcd_buf);
    lcd.setCursor(0,1);
    
      // if we're not displaying only a value, and there's
      // another menu entry to display -- display it on the 
      // second line..
      
    if( ! value_only ) {
      if( cur_pos + 1 <= max_menu[cur_menu] ) {
        cur_pos_sel = cur_pos + 1;
        memset(lcd_buf, ' ', sizeof(char) * MAX_LCD_STR);
        strcpy_P(lcd_buf, (char*)pgm_read_word(&(these[cur_pos + 1])));
        lcd.print("  ");
        lcd.print(lcd_buf);
      }
        // clear out in value entry setting, if set
      ui_ctrl_flags &= (255-UC_Value_Entry-UC_Value_Drawn);
      
    }
    else {
      
        // display (and possibly set) the value of the current entry
      ui_ctrl_flags |= UC_Value_Entry;
      
 
      if(! ( ui_ctrl_flags & UC_Value_Drawn ) ) {  // why not just get the value every time?????
         // have not just drawn this value

           // place value from variable into
           // temporary buffer
        get_value(cur_menu, cur_pos, false);
        ui_ctrl_flags |= UC_Value_Drawn;  // value has been displayed
      }


        // display the correct current
        // temporary input value
     
        if (ui_type == UT_Float) {
          lcd.print(cur_inp_float, (byte) 2);
        }
        else if (ui_type == UT_OnOff) {
          if (cur_inp_bool == true) {
            lcd.print("On");
          } 
          else {
            lcd.print("Off");
          }
        }
        else if (ui_type ==  UT_UpDn) {
          if (cur_inp_bool == true) {
            lcd.print("Up");
          } 
          else {
            lcd.print("Dn");
          }
        }
        else if (ui_type == UT_LtRt) {
          if (cur_inp_bool == true) {
            lcd.print("Rt");
          } 
          else {
            lcd.print("Lt");
          }
        }
        else if (ui_type == UT_IpmPct) {
          if (cur_inp_bool == true) {
            lcd.print("IPM");
          } 
          else {
            lcd.print("PCT");
          }
        }
        else if (ui_type == UT_ContSms) {
          if (cur_inp_bool == true) {
            lcd.print("Continuous");
          } 
          else {
            lcd.print("Interleave");
          }
        }
        else if (ui_type == UT_RotLin) {
          if (cur_inp_bool == true) {
            lcd.print("Rotary");
          } 
          else {
            lcd.print("Linear");
          }
        }
        else if (ui_type == UT_Angle) {
          if( cur_inp_long == 0 ) {
            lcd.print(0,DEC);
          }
          else if( cur_inp_long == 1 ) {
            lcd.print(45,DEC);
          }
          else {
            lcd.print(90,DEC);
          }
        }
        else if (ui_type == UT_AltIO) {  // for alt i/o inputs
					if( cur_inp_long == IT_Disabled ) {  // Input types
						lcd.print("Disabled");
					}
					else if( cur_inp_long == IT_Start ) {
						lcd.print("Start");
					}
					else if( cur_inp_long == IT_Stop ) {
						lcd.print("Stop");
					}
					else if( cur_inp_long == IT_Toggle ) {
						lcd.print("Toggle");
					}
					else if( cur_inp_long == IT_ExtInterval ) {
						lcd.print("Ext. Interval.");
					}
					else if( cur_inp_long == IT_OutBefore ) {
						lcd.print("Out Before");
					}
					else if( cur_inp_long == IT_OutAfter ){
						lcd.print("Out After");
					}
					else if(cur_inp_long == IT_OutBoth ) {
						lcd.print("Out Both");
					}
					else {
						lcd.print("Change Dir");
					}
        }
        else if (ui_type == UT_CalSpeed) { // cal speed inputs in gobal set menu
	        display_spd_ipm(cur_inp_long, 0);
        }
        else if (ui_type == UT_Park) {
          if (cur_inp_long == 0)
            lcd.print("Stopped");
          else if (cur_inp_long == 1)
            lcd.print("Parking");
        }
        else {
          lcd.print((unsigned long)cur_inp_long);
        }
      
    }
    
  } // end if( draw_all == true
        
  else {
    
      // do not need to re-draw the whole screen
      
      // move cursor down if we're not in
      // a value input screen
    if( ! (ui_ctrl_flags & UC_Value_Entry) ) {
      lcd.setCursor(0,0);
      lcd.print(' ');
      lcd.setCursor(0,1);
      lcd.print('>');
    }

  }
}


void ui_set_backlight(byte value) {

    // make sure to not use pwm on lcd bkl pin
    // if timer1 has been used at some point
  if( ! timer_used ) {
    analogWrite(LCD_BKL, value);
  }
  else {
    if( value > 0 ) {
      digitalWrite(LCD_BKL, HIGH);
    }
    else {
      digitalWrite(LCD_BKL, LOW);
    }
  }
}

/* 

 Menu history functions
 
*/
 
void push_menu(byte menu, byte pos) {
    // push the given entry to the end of the list
 byte i;
  for( i = 0; i < sizeof(hist_menu) / sizeof(hist_menu[0]); i++) {
    if( hist_menu[i] == 0 ) {
      hist_menu[i] = menu+1; // use non-zero value so we can find it again
      hist_menu_cur[i] = pos;
      break;
    }
  }
}

void pop_menu(byte& menu, byte& pos) {
 char i;
  menu = 0; // default case, if list is empty
  pos = 0;
  for( i = sizeof(hist_menu) / sizeof(hist_menu[0]) - 1; i >= 0 ; i--) {
    if( hist_menu[i] != 0 ) { // occupied?
      menu = hist_menu[i]-1; // remove offset
      pos = hist_menu_cur[i];
      hist_menu[i] = 0; // this space now available
      break;
    }
  }
}

void flush_menu() {
  memset(hist_menu, 0, sizeof(hist_menu) / sizeof(hist_menu[0]));
}

