/*

   "DollyShield" MX2
   
   (c) 2010 C.A. Church / Dynamic Perception LLC
   (c) 2014 W.B. Phelps / Meier-Phelps Consulting
   
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
    
    Modified 27Jan14 wbphelps - use structure for EE data, reduces code
    size by over 2,000 bytes!
 
 */
 
#ifndef DS_EEPROM_H_
#define DS_EEPROM_H_

struct __EECal
{
  byte EE_check1; // EE version check
  
  float m_cal_array[2][3][3][2]; // motor calibrate data
  float m_cal_constant[2]; // motor calibration constants
  byte m_cal_speed[2]; // motor calibration points
  
  byte EE_check2; // EE version check
};

struct __EESettings
{
  byte EE_check1; // EE version check
  
  float cam_interval; // intervalometer time (seconds)
  unsigned long exp_tm; // camera exposure time
  unsigned int focus_tap_tm; // tap focus before exposing
  unsigned int post_delay_tm; // delay after exposing (mS)
  unsigned int cam_max; // max camera shots
  unsigned int cam_rpt_dly; // delay between camera repeat cycles
  boolean focus_shutter; // bring focus pin high w/ shutter
  byte cam_repeat; // camera repeat shots
  
  unsigned int m_speeds[2][2]; // motor speeds, both manual and auto
  byte m_dirs[2][2]; // current motor direction
  float m_diarev[2]; // distance (i) per revolution
  float m_rpm[2]; // motor RPMs
  float max_ipm[2]; // calculated max ipm
  float min_ipm[2]; // user-configurable min ipm
  byte min_spd[2]; // minimum speed (min ipm->255 scale value)
  byte m_min_pulse[2]; // minimu pulse cycles per motor
  boolean ui_motor_display; // show ipm (true) or pct (false)
  boolean motor_mode; // motor mode is pulse (continuous) (true) or interleave (s-m-s) (false)
  byte m_ramp_set[2]; // ramping data
//  float m_cal_array[2][3][3][2]; // motor calibrate data
//  float m_cal_constant[2]; // motor calibration constants
//  byte motor_spd_cal[2]; // motor calibration points
  unsigned int m_maxsms[2]; // maximum sms distance
  byte m_angle[2]; // user set angle
  unsigned int m_lead_in[2]; // lead-in for axis movement
  unsigned int m_lead_out[2]; // lead-out for axis movement
  
  unsigned int lcd_dim_tm;
  boolean blank_lcd;
  byte input_type[2];
  boolean ui_is_metric;
  unsigned long ext_trig_pre_delay;
  unsigned long ext_trig_pst_delay;
  
  float merlin_man_spd[2];
  boolean merlin_enabled;
  boolean gb_enabled; // usb trigger flag
  byte altio_dir;  // default alt I/O rising/falling direction
  
  boolean ui_invdir;
  byte cur_bkl;
  boolean alt_menu; // alternate menu buttons - left=back, right=select
  boolean ul_cursor; // block, underline
  
  byte EE_check2; // EE version check
};

void ee_load(void);
void ee_save(void);
extern struct __EECal EECal; 
extern struct __EESettings EE;

#endif

