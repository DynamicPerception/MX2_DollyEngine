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
  UI Screen-drawing functions
  ========================================
  
*/


void prep_home_screen() {
  lcd.clear();
  lcd.setCursor(0,0);

  if( run_status & B10000000 ) {
      // in 'external intervalometer' mode, show 'ext' inseatd of 'on'
    if( external_interval & B11000000 ) {
      lcd.print("Ext");
    }
    else {
      lcd.print("On");
    }
  }
  else {
    lcd.print("Off");
  }

  lcd.setCursor(4, 0);
}

void show_merlin_home() {

  lcd.print("  Scope");  

  lcd.setCursor(0,1);
  
  if( merlin_dir[0] == 1 ) {
    lcd.print('L');
  }
  else {
    lcd.print('R');
  }


 display_spd_merlin(merlin_speeds[0], 0);

 
  lcd.setCursor(8,1);
  
  
  if( merlin_dir[1] == 1 ) {
    lcd.print('D');
  }
  else {
    lcd.print('U');
  }

 display_spd_merlin(merlin_speeds[1], 1);

    // we call this here mainly to reset the
    // cursor position when in an input
  if( main_scr_input ) 
    get_merlin_set(main_scr_input, false);

}


void show_home() {
  
  prep_home_screen();  
  
  if( merlin_flags & B00010000 ) {
    // show merlin screen instead
    show_merlin_home();
    return;
  }
  
    // deal with interval times that are less than total time
    // required between shots
  float i_total = calc_total_cam_tm();
  
  if( cam_interval < i_total ) {
    lcd.print(i_total, 1);
  }
  else {
    lcd.print((float) cam_interval, 1);
  }
  
  lcd.print("s ");
 
  
  if( shots > 999 ) {
    lcd.setCursor(10,0);
  }
  else if( shots > 99 ) {
    lcd.setCursor(11, 0);
  }
  else if( shots > 9 ) {
    lcd.setCursor(12, 0);
  }
  else {
    lcd.setCursor(13,0);
  }

  lcd.print('[');
  lcd.print(shots, DEC);
  lcd.print(']');
 
  lcd.setCursor(0,1);
  
  if( m_wasdir[0] == 1 ) {
    lcd.print('L');
  }
  else {
    lcd.print('R');
  }

 
 if( m_type[0] == 1 ) {
   display_spd_deg(m_speeds[0], 0);
 }
 else if( ui_motor_display ) {
  // display pct 
   display_spd_ipm(m_speeds[0], 0);
 }
 else {
  display_spd_pct(m_speeds[0]);
 }

 
  lcd.setCursor(8,1);
  
  
  if( m_wasdir[1] == 1 ) {
    lcd.print('L');
  }
  else {
    lcd.print('R');
  }

 if( m_type[1] == 1 ) {
   display_spd_deg(m_speeds[0], 1);
 }
 else if( ui_motor_display ) {
  // display pct 
   display_spd_ipm(m_speeds[1], 1);
 }
 else {
  display_spd_pct(m_speeds[1]);
 }

    // we call this here mainly to reset the
    // cursor position when in an input
  if( main_scr_input ) 
    get_mainscr_set(main_scr_input, false);
}


void main_screen_select(boolean dir) {
  
  if( main_scr_input == 0 && dir == true ) {
    lcd.blink();
  }
  
  if( dir ) {
    main_scr_input++;
  }
  else {
    main_scr_input--;
  }

    
    // merlin screen has five inputs, normal home
    // has six
    
  byte max_inputs = (merlin_flags & B00010000) ? 5 : 6;

    // exit main scr setup
  
  if( (dir == true && main_scr_input > max_inputs) ||
      (dir == false && main_scr_input == 0 ) ) {
    lcd.noBlink();
    main_scr_input = 0;
    return;
  }
  
 get_mainscr_set(main_scr_input, false);
}

      
void show_manual() {

 ui_ctrl_flags |= B00000100;

 lcd.clear();
 lcd.noBlink();

 lcd.setCursor(0, 0);

 if( cur_motor == 0 ) {
  lcd.print("Axis 1");
 }
 else {
  lcd.print("Axis 2");
 }
 
 lcd.setCursor(0, 1);
 lcd.print("Speed: ");

 if( ui_motor_display ) {
  // display ipm 
   display_spd_ipm(m_speeds[cur_motor], cur_motor);
 }
 else {
  display_spd_pct(m_speeds[cur_motor]);
 }
 
  
}

 
void show_calibrate() {

    // show the motor calibrate screen
    
 ui_ctrl_flags |= B00000001;

 lcd.clear();
 lcd.noBlink();

 lcd.setCursor(0,0);
 
 lcd.print("Cal M");
 lcd.print(cur_motor + 1, DEC);
 lcd.print(" [");
 
 byte angle = m_cur_cal * 45;
 
 lcd.print(angle, DEC);
 lcd.print(" Deg]");
 
}



void execute_calibrate() {

    // in calibration  
  ui_cal_scrn_flags |= B10000000;
    // floating point input
  ui_type_flags |= B10000000;

  ui_float_tenths = false;
 
  
  byte was_cur_pos = 0;
  byte completed = 0;   
  
    // sms calibration
  for( byte i = 0; i <= 1; i++ ) {
    float traveled = 0.01 * (max_ipm[cur_motor]);
    unsigned int runspd = 0.01 * m_maxsms[cur_motor];
    cur_inp_float = traveled;

    completed++;
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Running ");  
    lcd.print('[');
    lcd.print(completed, DEC);
    lcd.print(" of 6]");
    
      // sms moving in i dir
      // at 6% of total distance
    motor_run_calibrate(1, runspd, i);

    update_cal_screen();
        
    m_cal_done = false;

    while( m_cal_done == false ) {
        byte held = ui_button_check();
    }
    
    m_cal_array[cur_motor][m_cur_cal][0][i] = traveled / cur_inp_float;

  }

    // save this for now, to avoid calc problems
  byte was_smsfx[2] = { m_smsfx[0], m_smsfx[1] };
  m_smsfx[0] = 0; m_smsfx[1] = 0;
  
    // pulse calibration  
  for( byte c = 1; c <= 2; c++ ) {
    byte ths_spd = c == 1 ? motor_spd_cal[0] : motor_spd_cal[1];
    
    for( byte i = 0; i <= 1; i++ ) {
      float des_ipm = motor_calc_ipm(cur_motor, ths_spd, true);
      cur_inp_float = des_ipm;

      completed++;
      
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Running ");  
      lcd.print('[');
      lcd.print(completed, DEC);
      lcd.print(" of 6]");

        // pulse moving in i dir
      motor_run_calibrate(2, ths_spd, i);
    
      update_cal_screen();
      
      m_cal_done = false;
      
      while(  m_cal_done == false ) {
          byte held = ui_button_check();
      }
      
      m_cal_array[cur_motor][m_cur_cal][c][i] = ( cur_inp_float / des_ipm );
    }
  }

    // restore values
  m_smsfx[0] = was_smsfx[0]; m_smsfx[1] = was_smsfx[1];
  
  ui_cal_scrn_flags &= B01111111;
  ui_cal_scrn_flags |= B01000000;
  
   // save values to memory
   // handle m_cal_array in a sane manner
   // float m_cal_array[2][3][3][2] 
   // 2 * 3 * 3 * 2 * 4 = 144
   
  byte* p = (byte*)(void*)&m_cal_array;
  eeprom_write(71, *p, 144);
  
}

  
void update_cal_screen() {
  
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Dist Moved:");
    lcd.setCursor(0,1);
    
    lcd.print(cur_inp_float, 2);
}
