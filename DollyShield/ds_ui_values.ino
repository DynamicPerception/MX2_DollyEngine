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


void get_value( byte menu, byte pos, boolean save ) {

    // find the correct value to display for the current menu item
    // calls the necessary related function to handle value display/setting
  if( pos > max_menu[menu] )
    return;
    
    //set as non-floating point/bool by default
  ui_type = 0;
  
  switch(menu) {
    case MU_MANUAL:
      get_manual_select(pos);
      break;
    case MU_AXIS1:    
      get_m_axis_set(pos, save, 0);
      break;
    case MU_AXIS2:    
      get_m_axis_set(pos, save, 1);
      break;
   case MU_CAMERA:
      get_m_cam_set(pos, save);
      break;
   case MU_PARK:
      get_park_select(pos,save);
      break;
   case MU_RESET:
      get_reset_set(pos, save);
      break;
   case MU_SETTINGS:
       get_global_set(pos, save);
       break;
   case MU_SCOPE:
       get_scope_set(pos, save);
       break;
  }
  
  
 
}


void move_val(boolean dir) {

    // increase or decrease input value

  if( ui_type == UT_Float ) {
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
  else if( ui_type > 0 && ui_type <= UT_Boolean ) {
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
    
    if( ui_type == UT_Angle ) {
            // ceiling on certain values
       if (cur_inp_long > 2)
        cur_inp_long = 2;
    }
    else if( ui_type == UT_AltIO ) {
          // ceiling for alt i/o types
       if (cur_inp_long > 8)
        cur_inp_long = 8;
    }
    else if (ui_type == UT_Park) {
       if (cur_inp_long > 1)
         cur_inp_long = 1; 
			 ui_ctrl_flags &= (255-UC_Value_Drawn); // value has changed
    }

  } // end else long type...
	
  
}




void get_m_axis_set( byte pos, boolean save, byte motor ) {

    ui_type = 0;
    
      // set axis configurable values
      
    switch(pos) { 
      case 0:
          // set ramp value
        if( save == true ) {
          motor_set_ramp(motor, cur_inp_long); // validate new value and set ramping
          ee_save();
        }

        cur_inp_long = EE.m_ramp_set[motor];
        break;

      case 1:
          // set lead-in value
        if( save == true ) {
          EE.m_lead_in[motor] = cur_inp_long;
          ee_save();
        }

        cur_inp_long = EE.m_lead_in[motor];
        break;

      case 2:
          // set lead-out value
        if( save == true ) {
          EE.m_lead_out[motor] = cur_inp_long;
          ee_save();
        }

        cur_inp_long = EE.m_lead_out[motor];
        break;
        
      case 3:
        ui_type = UT_Float;
          // set rpm
        if( save == true ) {
          EE.m_rpm[motor] = cur_inp_float;
          motor_update_dist(motor, EE.m_rpm[motor], EE.m_diarev[motor]);
          ee_save();
        }
        
        cur_inp_float = EE.m_rpm[motor];
        break;


      case 4: 
      
          // doly angle (for calibration)
        ui_type = UT_Angle;
        
        if( save == true ) {
          EE.m_angle[motor] = cur_inp_long;
          ee_save();
        }
        
        cur_inp_long = EE.m_angle[motor];
        break;

        
      case 5:
          // calibrate motor
          get_calibrate_select(motor);
          break;
       
      case 6:
          // calibration constant
                
        ui_type = UT_Float;
        
        if( save == true ) {
          EECal.m_cal_constant[motor] = cur_inp_float;
          ee_save();
        }
        
        cur_inp_float = EECal.m_cal_constant[motor];
        break;
        
      case 7:
          // min ipm setting
        ui_type = UT_Float;
        if( save == true ) {
            
          EE.min_ipm[motor] = cur_inp_float;
          EE.min_spd[motor] = 255 * ( EE.min_ipm[motor] / EE.max_ipm[motor] );
          ee_save();
        } 
        cur_inp_float = EE.min_ipm[motor];

        break;
        
      case 8:
          // distance per revolution
        ui_type = UT_Float;
 
        if( save == true ) {
          EE.m_diarev[motor] = cur_inp_float;
          motor_update_dist(motor, EE.m_rpm[motor], EE.m_diarev[motor]);
          ee_save();
        }
        
        cur_inp_float = EE.m_diarev[motor];

        break;

      case 9:
          // motor min pulse
        if( save == true ) {
          
          if(cur_inp_long > 255)
            cur_inp_long = 255;
            
          EE.m_min_pulse[motor] = cur_inp_long;          
          ee_save();
        }
        cur_inp_long = EE.m_min_pulse[motor];
        break;
        

    }
    
}




void get_m_cam_set( byte pos, boolean save ) {

    // reset this flag
  ui_float_tenths = false;
  
  
  switch(pos) {
    case 0:
        // interval timer
        
      ui_type = UT_Float;
      ui_float_tenths = true;
      
      if( save == true ) { 
        EE.cam_interval = cur_inp_float;
        ee_save();
      }
      cur_inp_float = EE.cam_interval;
      break;
    case 1:
        // max shots
      if( save == true ) {
        EE.cam_max = cur_inp_long;
        ee_save();
      }
      cur_inp_long = EE.cam_max;
      break;
    case 2:
        // exposure time
      if( save == true ) { 
        EE.exp_tm = cur_inp_long;
        ee_save();
      }
      cur_inp_long = EE.exp_tm;
      break;    
    case 3:
        // post exp delay
      if( save == true ) { 
        EE.post_delay_tm = cur_inp_long;
        ee_save();
      }
      cur_inp_long = EE.post_delay_tm;
      break;
    case 4:
        // focus tap tm
      if( save == true ) {
        EE.focus_tap_tm = cur_inp_long;
        ee_save();
      }
      cur_inp_long = EE.focus_tap_tm;
      break;
    case 5:
        // focus w/ shutter
      ui_type = UT_OnOff;
      if( save == true ) {
        EE.focus_shutter = cur_inp_bool;
        ee_save();
      }
      cur_inp_bool = EE.focus_shutter;
      break;
   case 6:
       // camera repeat value
      if( save == true ) {
        cur_inp_long = cur_inp_long > 255 ? 255 : cur_inp_long;
        EE.cam_repeat = cur_inp_long;
        ee_save();
      }
      cur_inp_long = EE.cam_repeat;
      break;
  case 7:
      // camera repeat delay
      if( save == true) {
        EE.cam_rpt_dly = cur_inp_long;
        ee_save();
      }
      cur_inp_long = EE.cam_rpt_dly;
      break;
  }
}
     
     
void get_park_select(byte pos, boolean save) {
  // select Park from menu: left, right
  // set to "Parking" with up
  // set to "Stopped" with down
  // display changes to "Parked" when finished
    
  ui_ctrl_flags |= UC_Park;
  altio_connect(0, IT_Stop); // enable I/O switch
  m_mode = 1; // set motor speed index to manual 
  cur_motor = 0; // we only park the dolly
	ui_type = UT_Park;
	if (cur_inp_long == 0) {  // stop
		if( run_status & RS_Motors_Running) { // motors running?
			motor_control(0, false);  // stop motor
		  run_status &= (255-RS_Parking); // not parking now
		}
	}
  else if (cur_inp_long == 1) {  // run
		if( ! (run_status & RS_Motors_Running) ) { // motors stopped?
		  run_status |= RS_Parking;
			if (pos == 1)
				motor_dir(0, 0);  // set direction (right = 0)
			else
				motor_dir(0, 1);
			motor_control(cur_motor, true);  // start motor
		}
	}

}
     
     
void get_reset_set(byte pos, boolean save) {
  
  ui_type = UT_OnOff;
  if (pos == 0) {
      // reset calibration memory
      if( save == true ) {
        byte check = FIRMWARE_VERSION;
        if (cur_inp_bool) // reset memory?
          check = 0; // wipe out check bytes
        EECal.EE_check1 = check; // force reset of EE vars on restart
        EECal.EE_check2 = check; // force reset of EE vars on restart
        ee_save();
      }
      if ((EECal.EE_check1 == FIRMWARE_VERSION) && (EECal.EE_check2 == FIRMWARE_VERSION))
        cur_inp_bool = false; // checks match, no reset
      else
        cur_inp_bool = true; // checks do not match, memory will be reset
  }
  else { //if (pos == 1) 
      // reset settings memory
      if( save == true ) {
        byte check = FIRMWARE_VERSION;
        if (cur_inp_bool) // reset memory?
          check = 0; // wipe out check bytes
        EE.EE_check1 = check; // force reset of EE vars on restart
        EE.EE_check2 = check; // force reset of EE vars on restart
        ee_save();
      }
      if ((EE.EE_check1 == FIRMWARE_VERSION) && (EE.EE_check2 == FIRMWARE_VERSION))
        cur_inp_bool = false; // checks match, no reset
      else
        cur_inp_bool = true; // checks do not match, memory will be reset
  }

}


void get_scope_set(byte pos, boolean save) {

   // reset this flag
  ui_float_tenths = false;
  ui_type = UT_Float;
  switch(pos) {
    case 0:
      
      if( save == true ) {
        EE.merlin_man_spd[0] = cur_inp_float;
        EE.merlin_man_spd[0] = EE.merlin_man_spd[0] > 350.0 ? 350.0 : EE.merlin_man_spd[0];
        ee_save();
      }
      
      cur_inp_float = EE.merlin_man_spd[0];
      break;
   case 1:
      
      if( save == true ) {
        EE.merlin_man_spd[1] = cur_inp_float;
        EE.merlin_man_spd[1] = EE.merlin_man_spd[1] > 350.0 ? 350.0 : EE.merlin_man_spd[1];
        ee_save();
      }
      
      cur_inp_float = EE.merlin_man_spd[1];
      break;
  }
}

      

void get_global_set(byte pos, boolean save) {
  ui_type  = 0;
  
  switch(pos) {
      
   case 0:
     // Alternate Menu Mode
     
     ui_type = UT_OnOff;
     
     if( save == true ) {
       EE.alt_menu = cur_inp_bool;
       ee_save();
     }
     
     cur_inp_bool = EE.alt_menu;
     break;
  
    case 1:
          // motor display type
        ui_type = UT_IpmPct;      
        
        if( save == true ) {
          EE.ui_motor_display = cur_inp_bool;
	        ee_save();
        }
        
        cur_inp_bool = EE.ui_motor_display;
        break;

    case 2:
          // motor slow type
        ui_type = UT_ContSms;      
        
        if( save == true ) {
          EE.motor_mode = cur_inp_bool;
	        ee_save();
        }        
        
        cur_inp_bool = EE.motor_mode;
        break;

    case 3:
  
        // backlight level    
      if(save == true) {
        EE.cur_bkl = cur_inp_long > 255 ? 255 : cur_inp_long;
        ui_set_backlight(EE.cur_bkl);
        ee_save();
      }
      
      cur_inp_long = EE.cur_bkl;
      break;
      
   case 4:
   
       // lcd dim time
     if( save == true ) {
       EE.lcd_dim_tm = cur_inp_long;
       ee_save();
     }
     
     cur_inp_long = EE.lcd_dim_tm;
     break;
     
   case 5:
        // blank lcd   
      ui_type = UT_OnOff;
      
      if( save == true ) {
        EE.blank_lcd = cur_inp_bool;
        ee_save();
      }
      
      cur_inp_bool = EE.blank_lcd;
      break;
      
   case 6: 
       // input 1
       
      ui_type = UT_AltIO;
      
      if( save == true ) {
        EE.input_type[0] = cur_inp_long;
        ee_save();
        altio_connect(0, cur_inp_long);
      }
      
      cur_inp_long = EE.input_type[0];
      break;

   case 7: 
       // input 2
       
      ui_type = UT_AltIO;
      
      if( save == true ) {
        EE.input_type[1] = cur_inp_long;
        altio_connect(1, cur_inp_long);
        ee_save();
      }
      
      cur_inp_long = EE.input_type[1];
      break;
      
   case 8:
      // metric display
     ui_type = UT_OnOff;
     
     if( save == true ) {
       if ( cur_inp_bool != EE.ui_is_metric ) {
         
           // only convert values when the 
           // UI metric type changes
           
           
         if( EE.ui_is_metric ) {
           // going to imperial
           EE.m_diarev[0] = EE.m_diarev[0] / 2.54;
           EE.min_ipm[0]  = EE.min_ipm[0] / 2.54;
           EE.m_diarev[1] = EE.m_diarev[1] / 2.54;
           EE.min_ipm[1]  = EE.min_ipm[1] / 2.54;

         }
         else {
           // going to metric
           EE.m_diarev[0] *= 2.54;
           EE.min_ipm[0]  *= 2.54;
           EE.m_diarev[1] *= 2.54;
           EE.min_ipm[1]  *= 2.54;
         }
         
         EE.ui_is_metric = cur_inp_bool;
         ee_save(); // write values to memory
         
         motor_update_dist(0, EE.m_rpm[0], EE.m_diarev[0]);
         motor_update_dist(1, EE.m_rpm[1], EE.m_diarev[1]);
  
       }
       
     }
     
     cur_inp_bool = EE.ui_is_metric;
     break;
     
   case 9:
     // merlin enable
      ui_type = UT_OnOff;
      
      if( save == true ) {
        EE.merlin_enabled = cur_inp_bool;
        ee_save();
      }
      
      cur_inp_bool = EE.merlin_enabled;
      break;

   case 10:
     // low calibration spd
      ui_type = UT_CalSpeed;
      
      if( save == true ) {
        EECal.m_cal_speed[0] = cur_inp_long;
        ee_save();
      }
      
      cur_inp_long = EECal.m_cal_speed[0];
      break;

   case 11:
     // high calibration spd
      ui_type |= UT_CalSpeed;
      
      if( save == true ) {
        EECal.m_cal_speed[1] = cur_inp_long;
        ee_save();
      }
      
      cur_inp_long = EECal.m_cal_speed[1];
      break;
  
  case 12: 
    // alt output pre time
    
    if( save == true ) {
      EE.ext_trig_pre_delay = cur_inp_long;
      ee_save();
    }
    
    cur_inp_long = EE.ext_trig_pre_delay;
    break;
    
  case 13:
    // alt output post time
    
    if( save == true ) {
      EE.ext_trig_pst_delay = cur_inp_long;
      ee_save();
    }
    
    cur_inp_long = EE.ext_trig_pst_delay;
    break;
     
   case 14:
     // GB enable
      ui_type = UT_OnOff;
      
      if( save == true ) {
        EE.gb_enabled = cur_inp_bool;
        ee_save();
      }
      
      cur_inp_bool = EE.gb_enabled;
      break;

   case 15:
     // invert dir display
      ui_type = UT_OnOff;
      
      if( save == true ) {
        EE.ui_invdir = cur_inp_bool;
        ee_save();
      }
      
      cur_inp_bool = EE.ui_invdir;
      break;
      
   case 16:
     // flip I/O trigger type
     
     ui_type = UT_OnOff;
     
     if( save == true ) {
       EE.altio_dir = (cur_inp_bool == false) ? FALLING : RISING;
       ee_save();
     }
     
     cur_inp_bool = (EE.altio_dir == FALLING) ? false : true;
     break;
     
   case 17:
     // Cursor Style
     
     ui_type = UT_OnOff;
     
     if( save == true ) {
       bool ce = Cursor_Enabled; // remember current setting
       setBlink(false); // turn it off
       EE.ul_cursor = cur_inp_bool;
       setBlink(ce); // reset with new style
       ee_save();
     }
     
     cur_inp_bool = EE.ul_cursor;
     break;
  
  }
  
}


void get_mainscr_set(byte pos, boolean save) {

    // clear out previous on/off select
     
  ui_type   = 0;
  ui_float_tenths = false;

  if( merlin_flags & B00010000 ) {
    get_merlin_set(pos, save);
    return;
  }  

   switch(pos) {
    case 1:

        // on/off
        
      lcd.setCursor(0,0);
      
      if( save ) {
        if( cur_inp_bool > 0 ) {   
            // if set to positive value
       
         start_executing();
         
        }
        else {
          stop_executing();
        }
        
      }
        
      ui_type = UT_OnOff;
      
      cur_inp_bool = run_status >> 7; // RS_Running
      break;

    case 2:
    
        // set interval time
      lcd.setCursor(4, 0);
      
      ui_type = UT_Float;
      ui_float_tenths = true;
      
      if( save ) {
        EE.cam_interval = cur_inp_float;
        ee_save();
      }

      cur_inp_float = EE.cam_interval;
      break;
      
    case 3:

        // dir for m1
      lcd.setCursor(0,1);

      if( save )
        motor_dir(0, cur_inp_bool);
           
      ui_type = UT_LtRt;
      cur_inp_bool = EE.m_dirs[m_mode][0];
      break;

    case 4:
        // spd for m1

      lcd.setCursor(1,1);

      if( ! EE.motor_mode ) {
          // shoot-move-shoot?
          
        cur_inp_long = cur_inp_long > EE.m_maxsms[0] ? EE.m_maxsms[0] : cur_inp_long;
      } 
      else {       
        
        cur_inp_long = cur_inp_long > 255 ? 255 : cur_inp_long;
      }
      
      if( save ) {
        motor_set_speed(0, (unsigned int) cur_inp_long); 
        EE.m_speeds[m_mode][0] = m_speeds[m_mode][0];
        ee_save();
            // calculate speed change per shot for ramping
            // if needed - use function to update values
//        motor_set_ramp(0, EE.m_ramp_set[0]);  // this is done when motors are started
           
      }
      
      cur_inp_long = m_speeds[m_mode][0];
              
      break;

    case 5:
        // dir for m2
      lcd.setCursor(8,1);
 
      if( save )
        motor_dir(1, cur_inp_bool);
           
      ui_type = UT_LtRt;
      cur_inp_bool = EE.m_dirs[m_mode][1];
      break;
      
    case 6:
        // spd for m2
        
      lcd.setCursor(9,1);
      
     if( ! EE.motor_mode ) {
          // shoot-move-shoot?
          
        cur_inp_long = cur_inp_long > EE.m_maxsms[1] ? EE.m_maxsms[1] : cur_inp_long;
      } 
      else {        
        cur_inp_long = cur_inp_long > 255 ? 255 : cur_inp_long;
      }

      
      if( save ) {
        motor_set_speed(1, (unsigned int) cur_inp_long); // called to validate value
        EE.m_speeds[m_mode][1] = m_speeds[m_mode][1];
        ee_save();
            // calculate speed change per shot for ramping
            // if needed - use function to update values
//        motor_set_ramp(1, EE.m_ramp_set[1]); done by start_executing
      }

      cur_inp_long = m_speeds[m_mode][1];
      break;
   }
   

}


void get_merlin_set(byte pos, boolean save) {
  
  ui_type   = 0;
  ui_float_tenths = false;
  
  switch(pos) {
    case 1:

        // on/off
        
      lcd.setCursor(0,0);
      
      if( save ) {
        if( cur_inp_bool > 0 ) {   
            // if set to positive value
       
         start_executing();
         
        }
        else {
          stop_executing();
        }
        
      }
        
      ui_type = UT_OnOff;
      
      cur_inp_bool = run_status >> 7; // RS_Running
      break;

    case 2: 
      // dir for yaw

      lcd.setCursor(0,1);

      if( save )
        merlin_set_dir(0, cur_inp_bool);
      
      ui_type = UT_LtRt;
      cur_inp_bool = merlin_dir[0];
      break;      
      
   case 3:
     // speed for yaw
       lcd.setCursor(1,1);
       ui_type = UT_Float;
       ui_float_tenths = false;
     
        // degrees
      cur_inp_float = cur_inp_float > 360.0 ? 360.0 : cur_inp_float;
      
      if( save ) 
        merlin_set_speed(0, cur_inp_float); 
              
      cur_inp_float = merlin_speeds[0];
              
      break;  
    case 4: 
      // dir for pitch

      lcd.setCursor(8,1);

      if( save )
        merlin_set_dir(1, cur_inp_bool);
           
      ui_type = UT_LtRt;
      cur_inp_bool = merlin_dir[1];
      break;
      
   case 5:
     // speed for yaw
       lcd.setCursor(9,1);
       ui_type = UT_Float;
       ui_float_tenths = false;
 

        // degrees
      cur_inp_float = cur_inp_float > 360.0 ? 360.0 : cur_inp_float;
      
      if( save ) 
        merlin_set_speed(1, cur_inp_float); 
              
      cur_inp_float = merlin_speeds[1];
              
      break;  
  }

}



void get_manual_select(byte pos) {

  
    // set in manual mode
  ui_ctrl_flags |= UC_Manual;
  m_mode = 1; // set motor speed index to manual 

    // merlin manual screen
    
  if( pos == 2 ) {
    merlin_flags |= B00100000;
      // set merlin into high speed mode
      // AC/20120113: removed 
      // ratio is now set in merlin_move_manual()
      
   // merlin.setRatio(1, 3);
   // merlin.setRatio(2, 3);
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
  
  float cur_ipm = motor_calc_ipm(motor, spd, EE.motor_mode);
  lcd.print(cur_ipm, 2);
  
        // handle metric conversion
  if( EE.ui_is_metric ) {
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


void display_spd_merlin(unsigned int spd, byte motor) {

  lcd.print(merlin_speeds[motor], 2);
    
}


