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
  UI Value Handling/Lookup functions
  ========================================
  
*/


void get_value( byte menu, byte pos, boolean read_save ) {

    // find the correct value to display for the current menu item
    // calls the necessary related function to handle value display/setting
  if( pos > max_menu[menu] )
    return;
    
    //set as non-floating point/bool by default
  ui_type_flags = 0;
  
  switch(menu) {
    case 1:
      get_manual_select(pos);
      break;
    case 2:    
      get_m_axis_set(pos, read_save, 0);
      break;
    case 3:    
      get_m_axis_set(pos, read_save, 1);
      break;
   case 4:
      get_m_cam_set(pos, read_save);
      break;
   case 5:
       get_global_set(pos, read_save);
       break;
   case 6:
       get_scope_set(pos, read_save);
       break;
  }
  
  
 
}


void move_val(boolean dir) {

    // increase or decrease input value

  if( ui_type_flags & B10000000 ) {
      // float type

    
      // how much to add/remove each time?
      
    float mod = ui_float_tenths ? 0.1 : 0.01;
    mod *= (float) inp_val_mult;
        
      // floating point input
    if( dir == true ) {
        // increase value
      cur_inp_float += mod;
    }
    else {
      if( cur_inp_float < mod ) {
        cur_inp_float = 0.0;
      }
      else {
        cur_inp_float -= mod;
      }
      
    }

  }
  else if( ui_type_flags & B01111110 ) {
      // any boolean type

    cur_inp_bool = ! cur_inp_bool;
  }
  else {
    
      // unsigned long type
    unsigned long mod = (1 * inp_val_mult);
      // long input
    if( dir == true ) {
      cur_inp_long += mod;
    }
    else {
      if( cur_inp_long < mod ) {
        cur_inp_long = 0;
      }
      else {
        cur_inp_long -= mod;
      }
    } // end if dir not true
    
    if( ui_type_flags & B00000001 ) {
            // ceiling on certain values
        cur_inp_long = cur_inp_long > 2 ? 2 : cur_inp_long;
    }
    else if( ui_type_flags2 & B10000000 ) {
          // ceiling for alt i/o types
        cur_inp_long = cur_inp_long > 7 ? 7 : cur_inp_long;
    }

  } // end else long type...
  
}




void get_m_axis_set( byte pos, boolean read_save, byte motor ) {

    ui_type_flags = 0;
    ui_type_flags2 = 0;
    
      // set axis configurable values
      
    switch(pos) { 
      case 0:
          // set ramp value
        if( read_save == true ) {
          motor_set_ramp(motor, cur_inp_long);         
          eeprom_write(61, m_ramp_set[0]);
          eeprom_write(62, m_ramp_set[1]);
        }

        cur_inp_long = m_ramp_set[motor];
        break;

      case 1:
          // set lead-in value
        if( read_save == true ) {
          m_lead_in[motor] = cur_inp_long;
          eeprom_write(229, m_lead_in[0]);
          eeprom_write(231, m_lead_in[1]);
        }

        cur_inp_long = m_lead_in[motor];
        break;

      case 2:
          // set lead-out value
        if( read_save == true ) {
          m_lead_out[motor] = cur_inp_long;
          eeprom_write(233, m_lead_out[0]);
          eeprom_write(235, m_lead_out[1]);
        }

        cur_inp_long = m_lead_out[motor];
        break;
        
      case 3:
        ui_type_flags |= B10000000;
          // set rpm
        if( read_save == true ) {
          m_rpm[motor] = cur_inp_float;
          motor_update_dist(motor, m_rpm[motor], m_diarev[motor]);
          eeprom_write(32, m_rpm[0]);
          eeprom_write(36, m_rpm[1]);
        }
        
        cur_inp_float = m_rpm[motor];
        break;

      case 4:
          // fixed sms?
        ui_type_flags |= B01000000;
        
        if( read_save == true ) { 
          m_smsfx[motor] = cur_inp_bool;
          eeprom_write(54, m_smsfx[0]);
          eeprom_write(55, m_smsfx[1]);
        }
          
        cur_inp_bool = m_smsfx[motor] ;
        break;

      case 5: 
      
          // doly angle (for calibration)
        ui_type_flags |= B00000001;
        
        if( read_save == true ) {
          m_angle[motor] = cur_inp_long;
          eeprom_write(215, m_angle[0]);
          eeprom_write(216, m_angle[1]);
        }
        
        cur_inp_long = m_angle[motor];
        break;

        
      case 6:
          // calibrate motor
          get_calibrate_select(motor);
          break;
       
      case 7:
          // calibration constant
                
        ui_type_flags |= B10000000;
        
        if( read_save == true ) {
          m_cal_constant[motor] = cur_inp_float;
          eeprom_write(239 + (motor * 4), m_cal_constant[motor]);
        }
        
        cur_inp_float = m_cal_constant[motor];
        break;
        
      case 8:
          // min ipm setting
        ui_type_flags |= B10000000;
        if( read_save == true ) {
            
          min_ipm[motor] = cur_inp_float;
          min_spd[motor] = 255 * ( min_ipm[motor] / max_ipm[motor] );
          eeprom_write(40, min_ipm[0]);
          eeprom_write(44, min_ipm[1]);
          eeprom_write(48, min_spd[0]);
          eeprom_write(49, min_spd[1]);
        } 
        cur_inp_float = min_ipm[motor];

        break;
        
      case 9:
          // distance per revolution
        ui_type_flags |= B10000000;
 
        if( read_save == true ) {
          m_diarev[motor] = cur_inp_float;
          motor_update_dist(motor, m_rpm[motor], m_diarev[motor]);
          eeprom_write(16, m_diarev[0]);
          eeprom_write(20, m_diarev[1]);  
        }
        
        cur_inp_float = m_diarev[motor];

        break;

      case 10:
          // motor min pulse
        if( read_save == true ) {
          m_min_pulse[motor] = cur_inp_long;
          eeprom_write(50, m_min_pulse[0]);
          eeprom_write(51, m_min_pulse[1]);
        }
        cur_inp_long = m_min_pulse[motor];
        break;
        
      case 11:
          // axis type
        ui_type_flags |= B00000010;
        if( read_save == true ) {
          m_type[motor] = cur_inp_bool;
          eeprom_write(52, m_type[0]);
          eeprom_write(53, m_type[1]);
        }
        cur_inp_bool = m_type[motor];
        break;
    }
    
}




void get_m_cam_set( byte pos, boolean read_save ) {

    // reset this flag
  ui_float_tenths = false;
  
  
  switch(pos) {
    case 0:
        // interval timer
        
      ui_type_flags |= B10000000;
      ui_float_tenths = true;
      
      if( read_save == true ) { 
        cam_interval = cur_inp_float;
        eeprom_write(67, cam_interval);
      }
      cur_inp_float = cam_interval;
      break;
    case 1:
        // max shots
      if( read_save == true ) {
        cam_max = cur_inp_long;
        eeprom_write(10, cam_max);
      }
      cur_inp_long = cam_max;
      break;
    case 2:
        // exposure time
      if( read_save == true ) { 
        exp_tm = cur_inp_long;
        eeprom_write(260, exp_tm);
      }
      cur_inp_long = exp_tm;
      break;    
    case 3:
        // post exp delay
      if( read_save == true ) { 
        post_delay_tm = cur_inp_long;
        eeprom_write(5, post_delay_tm);
      }
      cur_inp_long = post_delay_tm;
      break;
    case 4:
        // focus tap tm
      if( read_save == true ) {
        focus_tap_tm = cur_inp_long;
        eeprom_write(3, focus_tap_tm);
      }
      cur_inp_long = focus_tap_tm;
      break;
    case 5:
        // focus w/ shutter
      ui_type_flags |= B01000000;
      if( read_save == true ) {
        focus_shutter = cur_inp_bool;
        eeprom_write(7, focus_shutter);
      }
      cur_inp_bool = focus_shutter;
      break;
   case 6:
       // camera repeat value
      if( read_save == true ) {
        cur_inp_long = cur_inp_long > 255 ? 255 : cur_inp_long;
        cam_repeat = cur_inp_long;
        eeprom_write(249, cam_repeat);
      }
      cur_inp_long = cam_repeat;
      break;
  case 7:
      // camera repeat delay
      if( read_save == true) {
        cam_rpt_dly = cur_inp_long;
        eeprom_write(250, cam_rpt_dly);
      }
      cur_inp_long = cam_rpt_dly;
      break;
  }
}

void get_scope_set(byte pos, boolean read_save) {

   // reset this flag
  ui_float_tenths = false;
  
  
  switch(pos) {
    case 0:
      ui_type_flags |= B10000000;
      ui_float_tenths = true;
      
      if( read_save == true ) {
        merlin_man_spd[0] = cur_inp_float;
        merlin_man_spd[0] = merlin_man_spd[0] > 1440.0 ? 1440.0 : merlin_man_spd[0];
        eeprom_write(221, merlin_man_spd[0]);
      }
      
      cur_inp_float = merlin_man_spd[0];
      break;
   case 1:
      ui_type_flags |= B10000000;
      ui_float_tenths = true;
      
      if( read_save == true ) {
        merlin_man_spd[1] = cur_inp_float;
        merlin_man_spd[1] = merlin_man_spd[1] > 1440.0 ? 1440.0 : merlin_man_spd[1];
        eeprom_write(225, merlin_man_spd[1]);
      }
      
      cur_inp_float = merlin_man_spd[1];
      break;
  }
}

      

void get_global_set(byte pos, boolean read_save) {

  ui_type_flags  = 0;
  ui_type_flags2 = 0;
  
  switch(pos) {
  
    case 0:
          // motor display type
        ui_type_flags |= B00001000;      
        
        if( read_save == true ) {
          ui_motor_display = cur_inp_bool;
          eeprom_write(56, ui_motor_display);
        }
        
        cur_inp_bool = ui_motor_display;
        break;

    case 1:
          // motor slow type
        ui_type_flags |= B00000100;      
        
        if( read_save == true ) {
          motor_sl_mod = cur_inp_bool;
          eeprom_write(57, motor_sl_mod);
        }        
        
        cur_inp_bool = motor_sl_mod;
        break;

    case 2:
  
        // backlight level    
      if(read_save == true) {
        cur_bkl = cur_inp_long > 255 ? 255 : cur_inp_long;
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
      
      cur_inp_long = cur_bkl;
      break;
      
   case 3:
   
       // lcd dim time
     if( read_save == true ) {
       lcd_dim_tm = cur_inp_long;
       eeprom_write(58, lcd_dim_tm);
     }
     
     cur_inp_long = lcd_dim_tm;
     break;
     
   case 4:
        // blank lcd   
      ui_type_flags |= B01000000;
      
      if( read_save == true ) {
        blank_lcd = cur_inp_bool;
        eeprom_write(60, blank_lcd);
      }
      
      cur_inp_bool = blank_lcd;
      break;
      
   case 5: 
       // input 1
       
      ui_type_flags2 |= B10000000;
      
      if( read_save == true ) {
        altio_connect(0, cur_inp_long);
        eeprom_write(217,input_type[0]);
      }
      
      cur_inp_long = input_type[0];
      break;

   case 6: 
       // input 2
       
      ui_type_flags2 |= B10000000;
      
      if( read_save == true ) {
        altio_connect(1, cur_inp_long);
        eeprom_write(218,input_type[1]);
      }
      
      cur_inp_long = input_type[1];
      break;
      
   case 7:
      // metric display
     ui_type_flags |= B01000000;
     
     if( read_save == true ) {
       if ( cur_inp_bool != ui_is_metric ) {
         
           // only convert values when the 
           // UI metric type changes
           
           
         if( ui_is_metric ) {
           // going to imperial
           m_diarev[0] = m_diarev[0] / 2.54;
           min_ipm[0]  = min_ipm[0] / 2.54;
           m_diarev[1] = m_diarev[1] / 2.54;
           min_ipm[1]  = min_ipm[1] / 2.54;

         }
         else {
           // going to metric
           m_diarev[0] *= 2.54;
           min_ipm[0]  *= 2.54;
           m_diarev[1] *= 2.54;
           min_ipm[1]  *= 2.54;
         }
         
         ui_is_metric = cur_inp_bool;
           // write values to memory
         eeprom_write(219, ui_is_metric);
         
         eeprom_write(16, m_diarev[0]);
         eeprom_write(20, m_diarev[1]);  
         eeprom_write(40, min_ipm[0]);
         eeprom_write(44, min_ipm[1]);
         
         motor_update_dist(0, m_rpm[0], m_diarev[0]);
         motor_update_dist(1, m_rpm[1], m_diarev[1]);
  
       }
       
     }
     
     cur_inp_bool = ui_is_metric;
     break;
     
   case 8:
     // reset memory

     ui_type_flags |= B01000000;
     
     if( read_save == true ) {
       if( cur_inp_bool )
         eeprom_saved(false);
     }
     
     cur_inp_bool = false;
     break;
     
   case 9:
     // merlin enable
      ui_type_flags |= B01000000;
      
      if( read_save == true ) {
        merlin_enabled = cur_inp_bool;
        eeprom_write(220, merlin_enabled);
      }
      
      cur_inp_bool = merlin_enabled;
      break;

   case 10:
     // low calibration spd
      ui_type_flags2 |= B01000000;
      
      if( read_save == true ) {
        motor_spd_cal[0] = cur_inp_long;
        eeprom_write(237, motor_spd_cal[0]);
      }
      
      cur_inp_long = motor_spd_cal[0];
      break;

   case 11:
     // high calibration spd
      ui_type_flags2 |= B01000000;
      
      if( read_save == true ) {
        motor_spd_cal[1] = cur_inp_long;
        eeprom_write(238, motor_spd_cal[1]);
      }
      
      cur_inp_long = motor_spd_cal[1];
      break;
  
  case 12: 
    // alt output pre time
    
    if( read_save == true ) {
      ext_trig_pre_delay = cur_inp_long;
      eeprom_write(252, ext_trig_pre_delay);
    }
    
    cur_inp_long = ext_trig_pre_delay;
    break;
    
  case 13:
    // alt output post time
    
    if( read_save == true ) {
      ext_trig_pst_delay = cur_inp_long;
      eeprom_write(256, ext_trig_pst_delay);
    }
    
    cur_inp_long = ext_trig_pst_delay;
    break;
     
  }
  
  
  
}


void get_mainscr_set(byte pos, boolean read_save) {

    // clear out previous on/off select
     
  ui_type_flags   = 0;
  ui_type_flags2  = 0;
  ui_float_tenths = false;

  if( merlin_flags & B00010000 ) {
    get_merlin_set(pos, read_save);
    return;
  }  

   switch(pos) {
    case 1:

        // on/off
        
      lcd.setCursor(0,0);
      
      if( read_save ) {
        if( cur_inp_bool > 0 ) {   
            // if set to positive value
       
         start_executing();
         
        }
        else {
          stop_executing();
        }
        
      }
        
      ui_type_flags |= B01000000;
      
      cur_inp_bool = run_status >> 7;
      break;

    case 2:
    
        // set interval time
      lcd.setCursor(4, 0);
      
      ui_type_flags |= B10000000;
      ui_float_tenths = true;
      
      if( read_save ) {
        cam_interval = cur_inp_float;
        eeprom_write(67, cam_interval);
      }

      cur_inp_float = cam_interval;
      break;
      
    case 3:

        // dir for m1
      lcd.setCursor(0,1);

      if( read_save )
        motor_dir(0, cur_inp_bool);
           
      ui_type_flags |= B00010000;
      cur_inp_bool = m_wasdir[0];
      break;

    case 4:
        // spd for m1

      lcd.setCursor(1,1);

      if( ! motor_sl_mod ) {
          // shoot-move-shoot?
          
        cur_inp_long = cur_inp_long > m_maxsms[0] ? m_maxsms[0] : cur_inp_long;
      } 
      else {       
        
        cur_inp_long = cur_inp_long > 255 ? 255 : cur_inp_long;
      }
      
      if( read_save ) {
        motor_set_speed(0, (unsigned int) cur_inp_long); 
            // calculate speed change per shot for ramping
            // if needed - use function to update values
        motor_set_ramp(0, m_ramp_set[0]);
           
      }
      
      cur_inp_long = m_speeds[0];
              
      break;

    case 5:
        // dir for m2
      lcd.setCursor(8,1);
 
      if( read_save )
        motor_dir(1, cur_inp_bool);
           
      ui_type_flags |= B00010000;
      cur_inp_bool = m_wasdir[1];
      break;
      
    case 6:
        // spd for m2
        
      lcd.setCursor(9,1);
      
      if( m_smsfx[1] == true ) {
          // fixed shoot-move-shoot?
        cur_inp_long = cur_inp_long > m_maxsms[1] ? m_maxsms[1] : cur_inp_long;
      } 
      else {        
        cur_inp_long = cur_inp_long > 255 ? 255 : cur_inp_long;
      }

      
      if( read_save ) {
        motor_set_speed(1, (unsigned int) cur_inp_long); 
            // calculate speed change per shot for ramping
            // if needed - use function to update values
        motor_set_ramp(1, m_ramp_set[1]);
      }

      cur_inp_long = m_speeds[1];
      break;
   }
   

}


void get_merlin_set(byte pos, boolean read_save) {
  
  ui_type_flags   = 0;
  ui_type_flags2  = 0;
  ui_float_tenths = false;
  
  switch(pos) {
    case 1:

        // on/off
        
      lcd.setCursor(0,0);
      
      if( read_save ) {
        if( cur_inp_bool > 0 ) {   
            // if set to positive value
       
         start_executing();
         
        }
        else {
          stop_executing();
        }
        
      }
        
      ui_type_flags |= B01000000;
      
      cur_inp_bool = run_status >> 7;
      break;

    case 2: 
      // dir for yaw

      lcd.setCursor(0,1);

      if( read_save )
        merlin_set_dir(0, cur_inp_bool);
      
      ui_type_flags |= B00010000;
      cur_inp_bool = merlin_dir[0];
      break;      
      
   case 3:
     // speed for yaw
       lcd.setCursor(1,1);
       ui_type_flags |= B10000000;
       ui_float_tenths = true;
 

      
        // degrees
      cur_inp_float = cur_inp_float > 360.0 ? 360.0 : cur_inp_float;
      
      if( read_save ) 
        merlin_set_speed(0, cur_inp_float); 
              
      cur_inp_float = merlin_speeds[0];
              
      break;  
    case 4: 
      // dir for pitch

      lcd.setCursor(8,1);

      if( read_save )
        merlin_set_dir(1, cur_inp_bool);
           
      ui_type_flags |= B00010000;
      cur_inp_bool = merlin_dir[1];
      break;
      
   case 5:
     // speed for yaw
       lcd.setCursor(9,1);
       ui_type_flags |= B10000000;
       ui_float_tenths = true;
 

        // degrees
      cur_inp_float = cur_inp_float > 360.0 ? 360.0 : cur_inp_float;
      
      if( read_save ) 
        merlin_set_speed(1, cur_inp_float); 
              
      cur_inp_float = merlin_speeds[1];
              
      break;  
  }

}



void get_manual_select(byte pos) {

  
    // set in manual mode
  ui_ctrl_flags |= B00000100;

    // merlin manual screen
    
  if( pos == 2 ) {
    merlin_flags |= B00100000;
      // set merlin into high speed mode
    merlin.setRatio(1, 3);
    merlin.setRatio(2, 3);
      // show merlin screen
    show_merlin();
    return;
  }

    // set current motor
  cur_motor = pos;

    // show manual motor screen
  show_manual();


}
  
    
void get_calibrate_select(byte pos) {
    // display calibrate screen
  cur_motor = pos;  
  show_calibrate();
}

void display_spd_ipm(unsigned int spd, byte motor) {
  
  float cur_ipm = motor_calc_ipm(motor, spd, motor_sl_mod);
  lcd.print(cur_ipm, 2);
  
        // handle metric conversion
  if( ui_is_metric ) {
     lcd.print('c');
  }
  else {
    lcd.print('i');
  }
  
  
}

void display_spd_pct(byte spd) {
  float cur_pct =  (float) spd / (float) 255;
  cur_pct *= 100;
  
  if( cur_pct < 100 ) {
    lcd.print(cur_pct,1);
  }
   else {
     lcd.print((int) cur_pct, DEC);
   }
  lcd.print('%');
}

void display_spd_deg(int spd, byte motor) {
}


void display_spd_merlin(unsigned int spd, byte motor) {

  lcd.print(merlin_speeds[motor], 1);
    
}


