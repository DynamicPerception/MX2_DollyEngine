/* 

   "DollyShield" MX2
   
   (c) 2010 C.A. Church / Dynamic Perception LLC
   
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
  
*/


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
   
 lcd.print("(c) 2012 Dynamic");
 
 lcd.setCursor(5,1);
 lcd.print("Perception");

 delay(750);

 lcd.clear(); 

 lcd.setCursor(0,0); 
 lcd.print("MX2 Dolly Engine");
 lcd.setCursor(3,1);
 lcd.print("Version 0.92");
 
   // setup button input

 pinMode(BUT_PIN, INPUT);   
   // enable internal pull-up
 digitalWrite(BUT_PIN, HIGH);
 
 
   // set the update screen flag (draw main
   // screen)
 ui_ctrl_flags |= B10000000;
 
 delay(3000);
 
}


void check_user_interface() {
  

    // turn off/on lcd backlight if needed
  if( ui_ctrl_flags & B00000010 && input_last_tm < millis() - (lcd_dim_tm * 1000) ) {
    
    ui_ctrl_flags &= B11111101;
    
    if( blank_lcd ) 
      lcd.noDisplay();
      
    digitalWrite(LCD_BKL, LOW);
  }
  else if( ! (ui_ctrl_flags & B00000010) && input_last_tm > millis() - (lcd_dim_tm * 1000) ) {
    ui_ctrl_flags |= B00000010;
    
    lcd.display();
    
    analogWrite(LCD_BKL, cur_bkl);
  }
  
    // if we're set to update the display
    // (on-demand or once a second when not
    // in a menu)
  if( ui_ctrl_flags & B10000000 ||
      ( (ui_update_tm < millis() - 1000) && ! (ui_ctrl_flags & B01000000) ) ) {
  
      // determine whether to show home or manual screen      
    if( ! ( ui_ctrl_flags & B00000100 ) ) {
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
     
    ui_ctrl_flags &= B01111111;
    ui_update_tm = millis();
  }
  
  byte held = ui_button_check();
  
      // make sure to turn off motor if in manual
      // control and no button is held
  if( ui_ctrl_flags & B00000100 && held == false && run_status & B00010000 )
    motor_control(cur_motor, false);
 

}


byte ui_button_check() {

 static byte hold_but_cnt = 0;
  
 get_button_pressed();

 boolean held = false;
 
 for( byte i = BUT0; i <= BUT4; i++) {
   byte bt_press = is_button_press( i );
   
   if( bt_press == 0 )
     continue;
   
   if(  bt_press == 1 && millis() - input_last_tm > HOLD_BUT_MS ) {
       // button is pressed        

     hold_but_cnt  = 0;
     inp_val_mult  = 1;
     input_last_tm = millis();
     handle_input(i, false);

   }
   else if( bt_press == 2 ) {
     held = true;
     // button being held
     if( hold_but_tm <= millis() - HOLD_BUT_MS) {
       hold_but_tm   = millis();
       input_last_tm = millis();
               
       handle_input(i, true);
       
       hold_but_cnt++;
       
       if( hold_but_cnt >= 8 ) {
         inp_val_mult = inp_val_mult >= 1000 ? 1000 : inp_val_mult * HOLD_BUT_VALINC;
         hold_but_cnt = 0;
       }

     }

   } // end else if button press state == 2

 } // end for loop
 
 return(held);
 
}


void get_button_pressed() {
  // see which buttons are pressed
  // buttons are on one analog pin, the value
  // determines which button, if any is pressed

    // read analog input
  int val_read = analogRead(BUT_PIN - 14);

    // don't let it flip in a single read
  if( abs(last_but_rd - val_read) > BUT_THRESH ) {
    last_but_rd = val_read; 
    button_pressed = 0;
    return;
  }
  
  if( val_read > (BUT0_VAL - BUT_THRESH) && val_read < (BUT0_VAL + BUT_THRESH) ) {
    button_pressed = BUT0;
  }
  else if( val_read > (BUT1_VAL - BUT_THRESH) && val_read < (BUT1_VAL + BUT_THRESH) ) {
    button_pressed = BUT1;
  }
  else if( val_read > (BUT2_VAL - BUT_THRESH) && val_read < (BUT2_VAL + BUT_THRESH) ) {
    button_pressed = BUT2;
  }
  else if( val_read > (BUT3_VAL - BUT_THRESH) && val_read < (BUT3_VAL + BUT_THRESH) ) {
    button_pressed = BUT3;
  }
  else if( val_read > (BUT4_VAL - BUT_THRESH) && val_read < (BUT4_VAL + BUT_THRESH) ) {
    button_pressed = BUT4;
  }
  else {
    button_pressed = 0;
  }
  
  
}


byte is_button_press(byte button) {
  
    // determine if the given button was
    // pressed, held, or is neither

  static byte button_was = 0;
  
    // if the button is set as 'active'
  if( button_pressed == button ) {
      // if we have already registered a press without
      // registering a non-press
    if( button_was ) {
        // increase 'skip hold count'
      return(2);
    }
      // button was not previous pressed...
    button_was = button;
    hold_but_tm = millis();
    return(1);
  }
 
    // if button set as inactive
    
    // if button was previously set as active,
    // register previous state as inactive
  if( button_was == button ) {
    button_was = 0;
        // set button as not currently pressed
    button_pressed = 0;
  }
    
  return(0);
}
  
  
byte get_menu( byte mnu, byte pos ) {

  // where is our target menu when 
  // mnu.pos is pressed?


  switch(mnu) {
    case 0:
    
      if( pos <= 5 )
        return( pos + 1 );

      break;
    
    case 1:
        // manual control is special return code
      return(254);
      
    default:
      break;        
  }
      
 

  // default is 'no target', an input value
 return(255);
 
}

/*
   handle user input
*/

void handle_input( byte button, boolean held ) {
  
  // do what needs to be done when whatever
  // button is hit
  
  if( button == BUT_CT ) {
      // call center button function
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
  
  
void ui_button_center( boolean held ) {
      // center button

        // on calibration screen

     if( ui_ctrl_flags & B00000001 ) {
       
       if( ui_cal_scrn_flags & B01000000 ) {
         // completed calibrating
         ui_cal_scrn_flags &= B00111111;
         show_calibrate();
         return;
       }  
       else if( ui_cal_scrn_flags & B10000000 ) {
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
       lcd.noBlink();
       main_scr_input = 0;
     }
     
     
       // if in manual control
     if( ui_ctrl_flags & B00000100  ) {
         // clear out manual ctrl flag
       ui_ctrl_flags &= B11111011;
       
       merlin_flags &= B11011111;

         // resume back to setup menu
       ui_ctrl_flags |= B01000000;
       cur_menu = 1; // show manual menu again
       draw_menu(0, false);
       return;
     }


    if( ! (ui_ctrl_flags & B01000000) ) {
        // not in any setup menu
      ui_ctrl_flags |= B01000000;
      cur_menu = 0;
      draw_menu(0,false);
    }
    else {
       
      
        // in a setup menu, find
        // the next menu to go to
        
          // calibration, don't do anything else
          // EJD:20130329: Corregido && cur_pos == 6 que no dejaba acceder a calibrar la constante 
      if( (cur_menu == 2 || cur_menu == 3) && cur_pos == 5 ) {
        get_value(cur_menu, cur_pos, false);
        return;
      }


      byte new_menu = get_menu(cur_menu, cur_pos);

        // if drawing motor manual screen...
        
      if( new_menu == 254 ) {
        get_value(cur_menu, cur_pos, false);
        return;
      }
      
      
      if( new_menu == 255 && ! (ui_ctrl_flags & B00100000) )  {
            // this is not a menu, but an input of some
            // sort
                  
          draw_menu(3,true);
          return;
      }
      else if( ui_ctrl_flags & B00100000 ) {
          // exiting out of value entry (save...)
          // go to previous menu

          // clear the cursor position
        cur_inp_pos = 0;
          
          // read value back from input
        get_value(cur_menu, cur_pos, true);
        
          // reset the float tenths (vs 100ths) parameter
        ui_float_tenths = false;
        
          // clear in value setting flag
          // and the flag indicating that
          // we've already displayed this value
        ui_ctrl_flags &= B11001111;
        draw_menu(0,false);
      }
      else {

          // entering another menu
          
          // record the last menu we were at
        push_menu(cur_menu);

          // clear in value setting flag
        ui_ctrl_flags &= B11011111;

          // set menu to new menu
        cur_menu = new_menu;
        draw_menu(0,false);
      }


    }
}


void ui_button_down( boolean held ) {

        // on calibration screen

     if( ui_ctrl_flags & B00000001 ) {
         
       if( ui_cal_scrn_flags & B10000000 ) {
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
    if( ui_ctrl_flags & B00000100 ) {

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
    if( ! (ui_ctrl_flags & B01000000) & main_scr_input == 0 )
      return;

   if( main_scr_input > 0 ) {
     move_val(false);
       // save present value
     get_mainscr_set(main_scr_input, true);
       // set screen to update
     ui_ctrl_flags |= B10000000;
   }
   else if( ui_ctrl_flags & B00100000 ) {
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
     if( ui_ctrl_flags & B00000001 ) {
         
       if( ui_cal_scrn_flags & B10000000 ) {
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
    if( ui_ctrl_flags & B00000100 ) {

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
        motor_speed_adjust(cur_motor,1 + inp_val_mult, true);
        show_manual();
      }
      return;
    }
    
    if( ! (ui_ctrl_flags & B01000000) & main_scr_input == 0 ) {
      
        // alternate between merlin and home screens
        
      if( merlin_enabled ) {
          // switch between merlin and normal home screens
        if( merlin_flags & B00010000 ) {
          merlin_flags &= B11101111;
        }
        else {          
          merlin_flags |= B00010000;
        }

      return;
      
      }
    }
      
   if( main_scr_input > 0 ) {
    
     move_val(true);
       // save present value
     get_mainscr_set(main_scr_input, true);
     
       // set screen to update
     ui_ctrl_flags |= B10000000;
   }
   else if( ui_ctrl_flags & B00100000 ) {
        // entering a value
      move_val(true);
      draw_menu(3,true);
    }
    else {
      draw_menu(1,false);
    }

}

void ui_button_rt( boolean held ) {
  
     // clear out calibration screen value, if
     // set
    if( ui_ctrl_flags & B00000001 )
      ui_ctrl_flags &= B11111110;

      // if in manual control
    if( ui_ctrl_flags & B00000100 ) {

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
      else {
           // in manual
        if( held == true ) {
           // change motor direction
          motor_dir(cur_motor, 0);
          if( ! (run_status & B00010000) )
            motor_control(cur_motor, true);
          }
  
        show_manual();
      }
      
      return;
    }  


    if( ! (ui_ctrl_flags & B01000000) ) {
      // we're on main screen, rt switches value we can
      // adjust
      main_screen_select(true);
      return;
    }
    
    if( ui_ctrl_flags & B00100000 ) {
        // we're in a value entry mode.  Exit
        // entry without saving the value

        // clear in value setting flag
        // and the flag indicating that
        // we've already displayed this value
      ui_ctrl_flags &= B11001111;        
              // reset the float tenths (vs 100ths) parameter
      ui_float_tenths = false;

      draw_menu(0,false);
      return;
    }
    
      // draw previous menu
      
    if( cur_menu == 0 ) { 
        // we're at the highest menu, back to main screen 
      cur_pos = 0;  
        // clear setup flag
        // indicate display needs updating
      ui_ctrl_flags &= B10111111;
      ui_ctrl_flags |= B10000000;
        // clear out list of menus
      flush_menu();
    }
    else {
        // a parent menu can be drawn
      cur_menu = pop_menu();
      draw_menu(0,false);
    }
  

}


void ui_button_lt(boolean held) {
        // if in manual control
    if( ui_ctrl_flags & B00000100 ) {

      
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
           // change motor direction
          motor_dir(cur_motor, 1);
            // get motor moving (if not already)
          if( ! (run_status & B00010000) )
            motor_control(cur_motor, true);
        }

      show_manual();
      }

      return;
    }  

    if( ! (ui_ctrl_flags & B01000000) ) {
      // we're on main screen, lt switches value we can
      // adjust
      main_screen_select(false);
      return;
    }

    // left button
    if( ! (ui_ctrl_flags & B01000000) || ui_ctrl_flags & B00100000 )
      return;
}



/* 
  Draw screens
*/


void draw_menu(byte dir, boolean value_only) {

  
    // turn off blinking, if on...
  lcd.noBlink();
  
  boolean draw_all = false;

  // determine the direction we are going, up/down (1/2),
  // draw all (but don't move position) (3), and draw
  // new menu from top (0)
  
  if( dir == 2 ) {
      // down
    cur_pos++;
    if( cur_pos > cur_pos_sel ) {
      lcd.clear();
      draw_all = true;
    }
  }    
  else if( dir == 1 ) {
      // up
    cur_pos = cur_pos == 0 ? 0 : cur_pos - 1;
    
    if( cur_pos < cur_pos_sel ) {
      lcd.clear();
      draw_all = true;
    }
  }
  else if( dir == 3 ) {
      // draw all (no move)
    draw_all = true;
  }
  else {
      // draw new menu (from top)
    cur_pos = 0;
    draw_all = true;
  }
  
    // don't overrun the memory locations for this menu
    
  cur_pos = cur_pos > max_menu[cur_menu] ? max_menu[cur_menu] : cur_pos;
  
   switch( cur_menu ) {
    
    case 0:
    
      draw_values(menu_str, draw_all, value_only);
      break;
      
    case 1:
    
      draw_values(man_str, draw_all, value_only);
      break;
      
    case 2:
    
      draw_values(axis0_str, draw_all, value_only);
      break;
      
    case 3:
    
      draw_values(axis1_str, draw_all, value_only);
      break;

    case 4:

      draw_values(cam_str, draw_all, value_only);
      break;
  
    case 5:
    
      draw_values(set_str, draw_all, value_only);
      break;

    case 6:
    
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
      ui_ctrl_flags &= B11011111;
      
    }
    else {
      
        // display the value of the current entry
      ui_ctrl_flags |= B00100000;
      
 
      if(! ( ui_ctrl_flags & B00010000 ) ) {
         // have just drawn this value

           // place value from variable into
           // temporary buffer
        get_value(cur_menu, cur_pos, false);
         
        ui_ctrl_flags |= B00010000;
      }
      
   

        // display the correct current
        // temporary input value
        
      if( ui_type_flags2 & B10000000 ) {
            // for alt i/o inputs
            
        if( cur_inp_long == 0 ) {
          lcd.print("Disabled");
        }
        else if( cur_inp_long == 1 ) {
          lcd.print("Start");
        }
        else if( cur_inp_long == 2 ) {
          lcd.print("Stop");
        }
        else if( cur_inp_long == 3 ) {
          lcd.print("Toggle");
        }
        else if( cur_inp_long == 4 ) {
          lcd.print("Ext. Interval.");
        }
        else if( cur_inp_long == 5 ) {
          lcd.print("Out Before");
        }
        else if( cur_inp_long == 6 ){
          lcd.print("Out After");
        }
        else if(cur_inp_long == 7 ) {
          lcd.print("Out Both");
        }
        else {
          lcd.print("Change Dir");
        }
        return;
      }
      else if( ui_type_flags2 & B01000000 ) {
          // cal speed inputs in gobal set menu
        display_spd_ipm(cur_inp_long, 0);
        return;
      }
      
      switch(ui_type_flags) {
        case B10000000:
          lcd.print(cur_inp_float, (byte) 2);
          break;
        case B01000000:
          if (cur_inp_bool == true) {
           lcd.print("On");
          } 
          else {
             lcd.print("Off");
          }
          break;
        case B00100000:
          if (cur_inp_bool == true) {
           lcd.print("Up");
          } 
          else {
             lcd.print("Dn");
          }
          break;
        case B00010000:
          if (cur_inp_bool == true) {
           lcd.print("Rt");
          } 
          else {
             lcd.print("Lt");
          }
          break;
        case B00001000:
          if (cur_inp_bool == true) {
           lcd.print("IPM");
          } 
          else {
             lcd.print("PCT");
          }
          break;
        case B00000100:
          if (cur_inp_bool == true) {
           lcd.print("Pulse");
          } 
          else {
             lcd.print("Interleave");
          }
          break;
        case B00000010:
          if (cur_inp_bool == true) {
           lcd.print("Rotary");
          } 
          else {
             lcd.print("Linear");
          }
          break;
        case B00000001:
          if( cur_inp_long == 0 ) {
            lcd.print(0,DEC);
          }
          else if( cur_inp_long == 1 ) {
            lcd.print(45,DEC);
          }
          else {
            lcd.print(90,DEC);
          }
          break;
        default:
          lcd.print((unsigned long)cur_inp_long);
          return;
      }
      
      
      
    }
    
  } // end if( draw_all == true
        
  else {
    
      // do not need to re-draw the whole screen
      
      // move cursor down if we're not in
      // a value input screen
    if( ! (ui_ctrl_flags & B00100000) ) {
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
    analogWrite(LCD_BKL, cur_bkl);
  }
  else {
    if( cur_bkl > 0 ) {
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
 
void push_menu(byte this_menu) { 
    // push the given entry to the end of the list
  for( byte i = 0; i < sizeof(hist_menu) / sizeof(hist_menu[0]); i++) {
    if( hist_menu[i] == 0 ) {
      hist_menu[i] = this_menu;
    }
  }
}

byte pop_menu() {
  byte bk_menu = 0;
  for( byte i = sizeof(hist_menu) / sizeof(hist_menu[0]); i > 0 ; i--) {
    if( hist_menu[i-1] != 0 ) {
      bk_menu = hist_menu[i-1];
      hist_menu[i-1] = 0;
    }
  }
  return(bk_menu);
}

void flush_menu() {
  memset(hist_menu, 0, sizeof(hist_menu) / sizeof(hist_menu[0]));
}


