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

/*

  ========================================
  EEPROM write/read functions
  ========================================
  
*/

#include "ds_eeprom.h"

#include <avr/eeprom.h>
#include <EEPROM.h>

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

const float c_m_diarev=3.53;
const float c_m_rpm=8.75;
const float c_min_ipm=20.0;
const float c_max_ipm=c_m_diarev*c_m_rpm;
const byte c_min_spd=(c_min_ipm/c_max_ipm)*255;
const unsigned int c_m_maxsms=c_max_ipm*100;

__EECal EECal;
__EESettings EE;

#define EECal_addr 32 // EE prom start address for calibration data
#define EESet_addr EECal_addr + sizeof(EECal)

__EECal EECal_default = {
 EE_VERSION, // EE structure version;
 { { { {0.61914329,0.61914329},{1.0,1.0},{2.01133251,2.11453032} }, 
     { {0.61914329,0.61914329},{1.0,1.0},{2.01133251,2.11453032} },
     { {0.61914329,0.61914329},{1.0,1.0},{2.01133251,2.11453032} } }, 
   { { {0.61914329,0.61914329},{1.0,1.0},{2.01133251,2.11453032} },
     { {0.61914329,0.61914329},{1.0,1.0},{2.01133251,2.11453032} },
     { {0.61914329,0.61914329},{1.0,1.0},{2.01133251,2.11453032} } }
 }, // m_cal_array[2][3][3][2]
 {0.69,0.69}, // m_cal_constant[2]
 {1,40}, // motor_spd_cal[2]
 EE_VERSION, // EE structure version;
};

__EESettings EESet_default = {
 EE_VERSION, // EE version;
 
 1.0, // cam_interval
 100, // exp_tm
 0, // focus_tap_tm
 0, // post_delay_tm
 0, // cam_max
 250, // cam_rpt_dly
 true, // focus_shutter
 0, // cam_repeat;
 
 {{0,0},{250,255}}, // m_speeds[2][2] // temp ???
 {{0,0},{0,0}}, // m_dirs[2][2]
 {c_m_diarev,c_m_diarev}, // m_diarev[2]
 {c_m_rpm,c_m_rpm}, // m_rpm[2]
 {c_max_ipm,c_max_ipm}, // max_ipm[2]
 {c_min_ipm,c_min_ipm}, // min_ipm[2]
 {c_min_spd,c_min_spd}, // min_spd[2]
 {125,125}, // min_pulse[2]
 true, // ui_motor_display
 true, // motor_mode continuous
 {0,0}, // m_ramp_set[2]
 {c_m_maxsms,c_m_maxsms}, // m_maxsms[2]
 {0,0}, // m_angle[2]
 {0,0}, // int m_lead_in[2]
 {0,0}, // int m_lead_out[2]
 
 5, // lcd_dim_tm
 false, // blank_lcd
 {IT_Disabled,IT_Disabled}, // input_type[2]
 false, // ui_is_metric
 0, // ext_trig_pre_dly
 0, // ext_trig_pst_dly
 
 {250.0, 250.0}, // merlin_man_spd[2]
 false, // merlin_enable
 false, // gb_enabled
 FALLING, // altio_dir
 
 false, // ui_invdir - direction inverted
 255, // cur_bkl;
 false, // alternate menu
 false, // underline cursor
 
 EE_VERSION, // EE version;
};

void ee_save()
{
uint8_t c1 = 0; // # of bytes written
	for (unsigned int p=0; p<sizeof(EECal); p++) {
		uint8_t b1 = EEPROM.read(EECal_addr + p);
		uint8_t b2 = *((uint8_t *) &EECal + p);
		if (b1 != b2) {
			EEPROM.write(EECal_addr + p, *((uint8_t*)&EECal + p));
			c1++;
		}
	}
	for (unsigned int p=0; p<sizeof(EE); p++) {
		uint8_t b1 = EEPROM.read(EESet_addr + p);
		uint8_t b2 = *((uint8_t *) &EE + p);
		if (b1 != b2) {
			EEPROM.write(EESet_addr + p, *((uint8_t*)&EE + p));
			c1++;
		}
	}
//	if (c1) {
//    Serial.print("ee updated "); Serial.println(c1);
//	}
}

void ee_read_cal(void)
{
	// read EE cal structure from EE
	for (unsigned int p=0; p<sizeof(EECal); p++) // read gloabls from EE
		*((uint8_t*)&EECal + p) = EEPROM.read(EECal_addr + p);
}
void ee_read_set(void)
{
	// read EE settings structure from EE
	for (unsigned int p=0; p<sizeof(EE); p++) // read gloabls from EE
		*((uint8_t*)&EE + p) = EEPROM.read(EESet_addr + p);
}

void ee_load(void) 
{

  ee_read_cal(); // read calibration data into structure
  ee_read_set(); // read settings data into structure
  lcd.clear();
  lcd.print("EE Cal Ver ");
	lcd.print(EECal.EE_check1);
  lcd.setCursor(0,1);
  lcd.print("EE Set Ver ");
	lcd.print(EE.EE_check1);
	delay(2000);
	lcd.clear();

	if ((EECal.EE_check1!=EE_VERSION) || (EECal.EE_check2!=EE_VERSION)) { // check if EE needs to be initialized
		for (unsigned int p=0; p<sizeof(EECal); p++) { // copy settings structure to EE memory
			EEPROM.write(EECal_addr + p, *((uint8_t*)&EECal_default + p));
		}
		lcd.print("EE Cal Wrt ");
		lcd.print(EE_VERSION);
		ee_read_cal(); 
		delay(2000);
	}

	if ((EE.EE_check1!=EE_VERSION) || (EE.EE_check2!=EE_VERSION)) { // check if EE needs to be initialized
		for (unsigned int p=0; p<sizeof(EE); p++) { // copy settings structure to EE memory
			EEPROM.write(EESet_addr + p, *((uint8_t*)&EESet_default + p));
		}
	  lcd.setCursor(0,1);
		lcd.print("EE Set Wrt ");
		lcd.print(EE_VERSION);
		ee_read_set();
		delay(2000);
	}
	
	m_speeds[0][0] = EE.m_speeds[0][0];
	m_speeds[0][1] = EE.m_speeds[0][1];
	m_speeds[1][0] = EE.m_speeds[1][0];
	m_speeds[1][1] = EE.m_speeds[1][1];

//  lcd.setCursor(0,1); 
//	lcd.print("EE loaded ");
//	lcd.print(EE.EE_check2);
//	delay(1000);
// Serial.print("ee size="); Serial.println(&EE.EE_check2-&EE.EE_check1+1);

}
