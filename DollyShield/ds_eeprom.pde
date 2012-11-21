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
  EEPROM write/read functions
  ========================================
  
*/


/* 

 *******************************
 Mapping of Data Positions in EEPROM memory
 *******************************

 (position count starts at zero)
 
 flash enabled   = 0
 (was exp_tm)    = 1-2*
 focus_tap_tm    = 3-4
 post_delay_tm   = 5-6
 focus_shutter   = 7
                 = 8-9
 cam_max         = 10-11
 m_speeds[0]     = 12-13
 m_speeds[1]     = 14-15
 m_diarev[0]     = 16-19
 m_diarev[1]     = 20-23
 max_ipm[0]      = 24-27
 max_ipm[1]      = 28-31
 m_rpm[0]        = 32-35
 m_rpm[1]        = 36-39
 min_ipm[0]      = 40-43
 min_ipm[1]      = 44-47
 min_spd[0]      = 48
 min_spd[1]      = 49
 m_min_pulse[0]  = 50
 m_min_pulse[0]  = 51
 m_type[0]       = 52
 m_type[0]       = 53
 m_smsfx[0]      = 54
 m_smsfx[1]      = 55 
 ui_motor_display = 56
 motor_sl_mod    = 57
 lcd_dim_tm      = 58-59
 blank_lcd       = 60
 m_ramp_set[0]   = 61
 m_ramp_set[0]   = 62
 m_maxsms[0]     = 63-64
 m_maxsms[1]     = 65-66
 cam_interval    = 67-70
 m_cal_array[]   = 71-214
 m_angle[0]      = 215
 m_angle[1]      = 216 
 input_type[0]   = 217
 input_type[1]   = 218
 ui_is_metric    = 219
 merlin_enable   = 220
 merlin_man_spd[0] = 221-224
 merlin_man_spd[1] = 225-228
 m_lead_in[0]    = 229-230
 m_lead_in[1]    = 231-232
 m_lead_out[0]   = 233-234
 m_lead_out[1]   = 235-236
 motor_spd_cal[0] = 237
 motor_spd_cal[1] = 238
 m_cal_constant[0] = 239-242
 m_cal_constant[1] = 243-246
 firmware_version  = 247-248
 cam_repeat     = 249
 cam_rpt_dly    = 250-251
 ext_trig_pre_dly = 252-255
 ext_trig_pst_dly = 256-259
 exp_tm         = 260-263
 
*/




boolean eeprom_saved() {
  
    // read eeprom saved status
    
  byte saved = EEPROM.read(0);
 
   // EEPROM memory is by default set to 1, so we
   // set it to zero if we've written data to eeprom
  return( ! saved );
}

void eeprom_saved( boolean saved ) {
  // set eeprom saved status
  
   // EEPROM memory is by default set to 1, so we
   // set it to zero if we've written data to eeprom
  
  EEPROM.write(0, !saved);
}





// One can ask why I didn't use the templates from http://www.arduino.cc/playground/Code/EEPROMWriteAnything
// The primary reason here is that we're going to be calling these functions OFTEN, and I _really_ don't 
// want the templates getting inlined _everywhere_, what a mess!  So, rather than be slick, let's just declare
// what we mean, and do it once - forget about the overhead of the function call, and worry more about
// flash and stack abuse 


void eeprom_write( int pos, byte& val, byte len ) {
  byte* p = (byte*)(void*)&val;
  for( byte i = 0; i < len; i++ )
    EEPROM.write(pos++, *p++);    

    // indicate that memory has been saved
  eeprom_saved(true);

}
    
void eeprom_write( int pos, unsigned int& val ) {
  byte* p = (byte*)(void*)&val;   
  eeprom_write(pos, *p, sizeof(int));  
}

void eeprom_write( int pos, unsigned long& val ) {
  byte* p = (byte*)(void*)&val;   
  eeprom_write(pos, *p, sizeof(long));    
}

void eeprom_write( int pos, float& val ) {
  byte* p = (byte*)(void*)&val;   
  eeprom_write(pos, *p, sizeof(float));    
}

void eeprom_write( int pos, byte& val ) {  
  EEPROM.write(pos, val);
    // indicate that memory has been saved
  eeprom_saved(true);
}





 // read functions

void eeprom_read( int pos, byte& val, byte len ) {
  byte* p = (byte*)(void*)&val;
  for(byte i = 0; i < len; i++) 
    *p++ = EEPROM.read(pos++);
}

void eeprom_read( int pos, byte& val ) {
  val = EEPROM.read(pos);
}


void eeprom_read( int pos, int& val ) {
  byte* p = (byte*)(void*)&val;
  eeprom_read(pos, *p, sizeof(int));
}

void eeprom_read( int pos, unsigned int& val ) {

  byte* p = (byte*)(void*)&val;
  eeprom_read(pos, *p, sizeof(int));
  
}

void eeprom_read( int pos, unsigned long& val ) {

  byte* p = (byte*)(void*)&val;
  eeprom_read(pos, *p, sizeof(long));
  
}

void eeprom_read( int pos, float& val ) {

  byte* p = (byte*)(void*)&val;
  eeprom_read(pos, *p, sizeof(float));
  
}
    
void write_all_eeprom_memory() {

    // write default values into eeprom
  eeprom_write(260, exp_tm);
  eeprom_write(3, focus_tap_tm);
  eeprom_write(5, post_delay_tm);
  eeprom_write(7, focus_shutter);

  eeprom_write(10, cam_max);
  eeprom_write(16, m_diarev[0]);
  eeprom_write(20, m_diarev[1]);
  eeprom_write(24, max_ipm[0]);
  eeprom_write(28, max_ipm[1]);
  eeprom_write(32, m_rpm[0]);
  eeprom_write(36, m_rpm[1]);
  eeprom_write(40, min_ipm[0]);
  eeprom_write(44, min_ipm[1]);
  eeprom_write(48, min_spd[0]);
  eeprom_write(49, min_spd[1]);
  eeprom_write(50, m_min_pulse[0]);
  eeprom_write(51, m_min_pulse[1]);
  eeprom_write(52, m_type[0]);
  eeprom_write(53, m_type[1]);
  eeprom_write(54, m_smsfx[0]);
  eeprom_write(55, m_smsfx[1]);
  eeprom_write(56, ui_motor_display);
  eeprom_write(57, motor_sl_mod);
  eeprom_write(58, lcd_dim_tm);
  eeprom_write(60, blank_lcd);
  eeprom_write(61, m_ramp_set[0]);
  eeprom_write(62, m_ramp_set[1]);
  eeprom_write(63, m_maxsms[0]);
  eeprom_write(65, m_maxsms[1]);
  eeprom_write(67, cam_interval);
  
   // handle m_cal_array in a sane manner
   // float m_cal_array[2][3][3][2] 
   // 2 * 3 * 3 * 2 * 4 = 144
   
  byte* p = (byte*)(void*)&m_cal_array;
  eeprom_write(71, *p, 144);
 
  eeprom_write(217, input_type[0]);
  eeprom_write(218, input_type[1]);
  eeprom_write(219, ui_is_metric);
  eeprom_write(220, merlin_enabled);
  eeprom_write(221, merlin_man_spd[0]);
  eeprom_write(225, merlin_man_spd[1]);
  eeprom_write(229, m_lead_in[0]);
  eeprom_write(231, m_lead_in[1]);
  eeprom_write(233, m_lead_out[0]);
  eeprom_write(235, m_lead_out[1]);
  eeprom_write(237, motor_spd_cal[0]);
  eeprom_write(238, motor_spd_cal[1]);
  eeprom_write(239, m_cal_constant[0]);
  eeprom_write(243, m_cal_constant[1]);

  eeprom_write(249, cam_repeat);
  eeprom_write(250, cam_rpt_dly);

  eeprom_write(252, ext_trig_pre_delay);
  eeprom_write(256, ext_trig_pst_delay);
  
}


 // restore memory
 
void restore_eeprom_memory() {

    // read eeprom stored values back into RAM
    
  eeprom_read(260, exp_tm); // moved from position 1
  eeprom_read(3, focus_tap_tm);
  eeprom_read(5, post_delay_tm);
  eeprom_read(7, focus_shutter);

  eeprom_read(10, cam_max);
  eeprom_read(16, m_diarev[0]);
  eeprom_read(20, m_diarev[1]);
  eeprom_read(24, max_ipm[0]);
  eeprom_read(28, max_ipm[1]);
  eeprom_read(32, m_rpm[0]);
  eeprom_read(36, m_rpm[1]);
  eeprom_read(40, min_ipm[0]);
  eeprom_read(44, min_ipm[1]);
  eeprom_read(48, min_spd[0]);
  eeprom_read(49, min_spd[1]);
  eeprom_read(50, m_min_pulse[0]);
  eeprom_read(51, m_min_pulse[1]);
  eeprom_read(52, m_type[0]);
  eeprom_read(53, m_type[1]);
  eeprom_read(54, m_smsfx[0]);
  eeprom_read(55, m_smsfx[1]);
  eeprom_read(56, ui_motor_display);
  eeprom_read(57, motor_sl_mod);
  eeprom_read(58, lcd_dim_tm);
  eeprom_read(60, blank_lcd);
  eeprom_read(61, m_ramp_set[0]);
  eeprom_read(62, m_ramp_set[1]);
  eeprom_read(63, m_maxsms[0]);
  eeprom_read(65, m_maxsms[1]);
  eeprom_read(67, cam_interval);
  
   // handle m_cal_array in a sane manner
   // float m_cal_array[2][3][3][2] 
   // 2 * 3 * 3 * 2 * 4 = 144
   
  byte* p = (byte*)(void*)&m_cal_array;
  eeprom_read(71, *p, 144);
 
  eeprom_read(217, input_type[0]);
  eeprom_read(218, input_type[1]);
  
  eeprom_read(219, ui_is_metric);
  
  eeprom_read(220, merlin_enabled);

  eeprom_read(221, merlin_man_spd[0]);
  eeprom_read(225, merlin_man_spd[1]);

  eeprom_read(229, m_lead_in[0]);
  eeprom_read(231, m_lead_in[1]);
  eeprom_read(233, m_lead_out[0]);
  eeprom_read(235, m_lead_out[1]);

  eeprom_read(237, motor_spd_cal[0]);
  eeprom_read(238, motor_spd_cal[1]);

  eeprom_read(239, m_cal_constant[0]);
  eeprom_read(243, m_cal_constant[1]);
  
  eeprom_read(249, cam_repeat);
  eeprom_read(250, cam_rpt_dly);

  eeprom_read(252, ext_trig_pre_delay);
  eeprom_read(256, ext_trig_pst_delay);
  
    // handle restoring alt input states
    
  if( input_type[0] != 0 )
    altio_connect(0,input_type[0]);

  if( input_type[1] != 0 )
    altio_connect(1,input_type[1]);
    
}



void eeprom_versioning() {
   // determine if eeprom version is correct 
   // so we can automatically flush saved memory 
   // when a new firmware is loaded 
   
 unsigned int eeprom_ver = 0;
 eeprom_read(247, eeprom_ver);

   // wipe out any saved eeprom settings
 if( eeprom_ver != FIRMWARE_VERSION ) {
  eeprom_ver = FIRMWARE_VERSION;
  eeprom_write(247, eeprom_ver);
    // order of operations is important, this must line
    // must happen after the eeprom_write function call, as
    // it automatically updates the eeprom saved status
  eeprom_saved(false);
 }  

   
}

