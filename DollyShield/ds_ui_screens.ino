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

  if( run_status & RS_Running ) {
      // in 'external intervalometer' mode, show 'ext' inseatd of 'on'
    if( external_interval & B11000000 || EE.gb_enabled == true ) {
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
  
  if( EE.cam_interval < i_total ) {
    lcd.print(i_total, 1);
  }
  else {
    lcd.print((float) EE.cam_interval, 1);
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
  
      // dir displays
//  char lt = EE.ui_invdir == true ? 'R' : 'L';
//  char rt = EE.ui_invdir == true ? 'L' : 'R';
  // wbp - display is the same, motion is different!
  if( EE.m_dirs[m_mode][0] == 1 ) {
    lcd.print('L');
  }
  else {
    lcd.print('R');
  }

	unsigned int mspd = m_speeds[m_mode][0];
 
  if( EE.ui_motor_display ) { 
     // display ipm 
    display_spd_ipm(mspd,0);
  }
  else {
    display_spd_pct(mspd);
  }
  
//  char md = ' '; // display motor direction if moving
//  if (run_status & RS_Motors_Running)
//    if (m_state[0] > 0)
//     md = "><"[EE.m_dirs[m_mode][0]];
//  lcd.print(md);
 
 
  lcd.setCursor(8,1);
  
  
  if( EE.m_dirs[m_mode][1] == 1 ) {
    lcd.print('L');
  }
  else {
    lcd.print('R');
  }

  mspd = m_speeds[m_mode][1];
  if( EE.ui_motor_display ) {
     // display ipm 
    display_spd_ipm(mspd,1);
  }
  else {
    display_spd_pct(mspd);
  }
  
//  md = ' ';
//  if (run_status & RS_Motors_Running)
//    if (m_state[1] > 0)
//      md = "><"[EE.m_dirs[m_mode][1]];
//  lcd.print(md);

    // we call this here mainly to reset the
    // cursor position when in an input
  if( main_scr_input ) 
    get_mainscr_set(main_scr_input, false);
}


void main_screen_select(boolean dir) {
  
  setBlink(true);
    // is Scope setting on?
  if( EE.merlin_enabled ) {
  
  }
    
    
    // merlin screen has five inputs, normal home has six
  byte max_inputs = 6;
  if (EE.merlin_enabled && merlin_flags & B00010000)
    max_inputs = 5;
  
  if( dir ) { // going right
    main_scr_input++;
    if( main_scr_input > max_inputs) {
      if (EE.merlin_enabled) {
        setBlink(false);
        main_scr_input = 0;  // dead spot to allow paging
        return;
      }
      else {
        main_scr_input = 1; // wrap around
        return;
      }
    }
  }
  else { // going left
    if (main_scr_input > 1)
      main_scr_input--;
    else {
      if (EE.merlin_enabled) {
        setBlink(false);
        main_scr_input = 0;  // dead spot to allow paging
        return;
      }
      else {
        main_scr_input = 6; // wrap around
        return;
      }
    }
  }

  get_mainscr_set(main_scr_input, false);

}

      
void show_manual() {

//  ui_ctrl_flags |= UC_Manual;  // wbp: it's already on

 lcd.clear();
 setBlink(false);

 lcd.setCursor(0, 0);

 if( cur_motor == 0 ) {
  lcd.print("Axis 1");
 }
 else {
  lcd.print("Axis 2");
 }
 
 lcd.setCursor(0, 1);
 lcd.print("Speed: ");

 unsigned int mspd = m_speeds[1][cur_motor];
 if( EE.ui_motor_display ) {
  // display ipm 
   display_spd_ipm(mspd, cur_motor);
 }
 else {
  display_spd_pct(mspd);
 }
 
// lcd.setCursor(15,1);
// char md = ' '; // display motor direction if moving
// if (m_state[cur_motor])
//   md = "><"[EE.m_dirs[1][cur_motor]];
// lcd.print(md);
 
}

 
void show_calibrate() {

    // show the motor calibrate screen
    
 ui_ctrl_flags |= UC_Calibrate;

 lcd.clear();
 setBlink(false);

// lcd.setCursor(0,0);
 
 lcd.print("Cal M");
 lcd.print(cur_motor + 1, DEC);
 lcd.print(" [");
 
 byte angle = m_cur_cal * 45;
 
 lcd.print(angle, DEC);
 lcd.print(" Deg]");
 
}



void execute_calibrate() {

    // in calibration  
  ui_cal_scrn_flags |= US_Calibrate;
    // floating point input
  ui_type = UT_Float;

  ui_float_tenths = false;
 
  
  byte was_cur_pos = 0;
  byte completed = 0;   
  
    // sms calibration
  for( byte i = 0; i <= 1; i++ ) {
    float traveled = 0.01 * (EE.max_ipm[cur_motor]);
    unsigned int runspd = 0.01 * EE.m_maxsms[cur_motor];
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
    
    EECal.m_cal_array[cur_motor][m_cur_cal][0][i] = traveled / cur_inp_float;

  }

  
    // pulse calibration  
  for( byte c = 1; c <= 2; c++ ) {
    byte ths_spd = c == 1 ? EECal.m_cal_speed[0] : EECal.m_cal_speed[1];
    
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
      
      EECal.m_cal_array[cur_motor][m_cur_cal][c][i] = ( cur_inp_float / des_ipm );
    }
  }

  
  ui_cal_scrn_flags &= (255-US_Calibrate);
  ui_cal_scrn_flags |= US_CalibrateDone;
  
   // save values to memory
   // handle m_cal_array in a sane manner
   // float m_cal_array[2][3][3][2] 
   // 2 * 3 * 3 * 2 * 4 = 144
   
   ee_save();
  
}

  
void update_cal_screen() {
  
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Dist Moved:");
    lcd.setCursor(0,1);
    
    lcd.print(cur_inp_float, 2);
}
