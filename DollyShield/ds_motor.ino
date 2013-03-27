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
  Motor control functions
  ========================================
  
*/


void motor_speed_adjust( byte motor, int val, boolean spd_floor ) {

   byte c_speed = 0;
    // val is expected to be between -255 and 255;

   if( (int) m_speeds[motor] + val >= 255 ) {
    c_speed = 255;
   }
   else {
     c_speed = (int) m_speeds[motor] + val >= 0 ? m_speeds[motor] + val : 0;
       // do we need to floor the value at the min speed setting? (man control)
     if( spd_floor ) 
       c_speed = c_speed < min_spd[motor] ? min_spd[motor] : c_speed;
   }

   motor_set_speed( motor, c_speed );   
   
}

void motor_control(byte motor, boolean state) {

   // turn motors on or off
 
  if( ! state ) {
      // set motors as not running

    unsigned int ths_spd = m_speeds[motor];
           
    motor_set_speed( motor, 0 );
 
    m_speeds[motor] = ths_spd;
    mcur_spds[motor] = ths_spd;

    run_status &= B11101111;
    
  }
  else {
      // set motors as running...
    run_status |= B00010000;
    if( mcur_spds[motor] > 0 )
      motor_set_speed(motor, mcur_spds[motor]);
  }
}



void motor_set_speed( byte motor, unsigned int m_speed ) {

  
  if( motor >= MAX_MOTORS )
    return;
    
  m_speeds[motor] = m_speed;
  m_sms_tm[motor] = 0;
  
        

  if( ! motor_sl_mod && ! (ui_ctrl_flags & B00000100) ) {

      // handle when in interleaved mode and not on
      // manual control screen
      
    float m_pct = ( (float) m_speed / (float) m_maxsms[motor] );  

    m_sms_tm[motor] = 60000.0 * m_pct;
    
    // calibrate
    m_sms_tm[motor] *= motor_cal_adjust(0,motor,0,m_wasdir[motor]);

  }
  else {
    
      // normalize to max pwm speed
    m_speed = m_speed > 255 ? 255 : m_speed;
      
    m_speeds[motor] = m_speed;
  }
  

    // do we need to go into pulsing mode?
    
  if( m_speed > 0 && m_speed < min_spd[motor]  ) {
      
    motor_calc_pulse_len(motor, m_speed, false);
    
  } //      
  else {
    on_pct[motor] = 0;
  }
  
  

  byte motor_pin = motor >= 1 ? MOTOR1_P : MOTOR0_P;
  
  if( ! (run_status & B00010000)  ) {
    // if disabled, do not move motor, but
    // instead adjust stored speed
    mcur_spds[motor] = m_speed;
    return;
  }
  else if( ! (ui_ctrl_flags & B00000100) && m_sms_tm[motor] > 0 ) {
      // just in case
    digitalWrite(motor_pin, LOW);
      // return if we're in an SMS condition
      // and not in manual mode
    return;
  }

    // if we've made it this far, set motor pin
    // to given analog speed

    // only set analog speed if it exceeds min speed

  if( m_speed >= min_spd[motor] ) {
    analogWrite(motor_pin, m_speed);
  }
  else {
    // just in case... switching down from 
    // pwm to pulsed...
    digitalWrite(motor_pin,LOW);
  }
  
}

void motor_calc_pulse_len(byte motor, unsigned int m_speed, boolean ignore_cal) {
    // for how many periods should the motor be on and
    // off? (pulsing mode)     

    float m_pct   = ( (float) m_speed / 255.0 );
    float periods = 1333.0; // 25 times a second 
    
    on_pct[motor] = periods * m_pct; 
    off_pct[motor] = (float) (periods - on_pct[motor]);
      
    float cal_amt = motor_cal_adjust(1,motor,m_speed, m_wasdir[motor]);
    
      // calibrate, if desired
    if( ! ignore_cal && cal_amt != 1.0 )
      off_pct[motor] = ( (double) off_pct[motor] * ( cal_amt * m_cal_constant[motor] )  ); 
        
    if(on_pct[motor] < 1)
      on_pct[motor] = 1;

    // make sure that we're on for a minimum amount of time
    
   if ( on_pct[motor] != 0 && on_pct[motor] < m_min_pulse[motor] ) {
       // adjust so that off time is increased relative to on time
     float diff = (float) m_min_pulse[motor] / (float) on_pct[motor];       
     off_pct[motor] = ((float) off_pct[motor] * diff);
     on_pct[motor] = m_min_pulse[motor];
   }
 
}

void motor_dir( byte motor, byte dir ) {
  
  if( m_wasdir[motor] == dir )
    return;
 
   // get current speed for the motor
  byte ths_speed = m_speeds[motor];
    
    // find direction pin
  byte m_dirp = motor >= 1 ? MOTOR1_DIR : MOTOR0_DIR;

    // reverse direction of motor #1, as the hardware
    // has the directions reversed.
    
  byte m_dirc = motor >= 1 ? !dir : dir;
  
  if( ths_speed > 0 ) {
      // motor was already moving, need to stop
      // and let motor settle before moving
      // stop motor
    motor_set_speed( motor, 0 );
    delay(100);    
  }

  
  digitalWrite(m_dirp, m_dirc);  
  m_wasdir[motor] = dir;
  motor_set_speed( motor, ths_speed );
}
  

float motor_calc_ipm(byte motor, unsigned int spd, boolean ths_mode) {
  // calculate ipm for a given speed and mode

    // get max speed for either pulse or sms mode
    // on calibration screen, always ch

  float maxspd = ( ! ths_mode ) ? (float) m_maxsms[motor] : 255.0;
  
    // in manual mode, we're always in 0-255 mode
  if( ui_ctrl_flags & B00000100 ) 
    maxspd = 255.0;
    
  float cur_ipm = (float) max_ipm[motor] * ( (float) spd / (float) maxspd );      
  
  return(cur_ipm);
}


void motor_update_dist(byte motor, float rpm, float diarev ) {
    // set distance settings when rpm or diarev change
    
  max_ipm[motor] = rpm * diarev;
  min_spd[motor] = 255 * ( min_ipm[motor] / max_ipm[motor] );
  m_maxsms[motor] = max_ipm[motor] * 100;
  
  eeprom_write(24, max_ipm[0]);
  eeprom_write(28, max_ipm[1]);
  eeprom_write(48, min_spd[0]);
  eeprom_write(49, min_spd[1]);
  eeprom_write(63, m_maxsms[0]);
  eeprom_write(65, m_maxsms[1]);

}

void motor_pulse() {

    // this function is called by timer1 to pulse motors
    // on and off in pulsing mode
    
  if( ! timer_engaged )
    return;
        
  volatile static byte mstate[2] = {0,0};
  volatile static unsigned long pulses[2]  = {1,1};

  volatile static byte pos = 0;
  
  pos++;
  
  
  for( byte i = 0; i < MAX_MOTORS; i++ ) {
      if( on_pct[i] > 0 ) {
      // speed is below min cont. speed
      
        if( ! mstate[i] ) {
          if( pulses[i] < off_pct[i] ) {
            pulses[i]++;
            continue;
          }
          else {            
              // set port value high for given motor
            PORTD |= (B00100000 << i);  
            //analogWrite(5, 165);
            mstate[i] = 1;
            pulses[i] = 1;
          }
        }
        else {
          if( pulses[i] < on_pct[i] ) {
            pulses[i]++;
            continue;
          }
          else {
            
              // set port value low for given motor
            //analogWrite(5, 0);
            PORTD &= ( B11111111 ^ ( B00100000 << i ) ); 
            mstate[i] = 0;
            pulses[i] = 1;
          }
        } // end else not mstate...
      }  // end if on_pct...
  }  // end for
    
}



void run_motor_sms(byte motor) {

 if( motor >= MAX_MOTORS ) {
   return;
 }

  cur_motor = motor;
  motor = motor >= 1 ? MOTOR1_P : motor;
  motor = motor == 0 ? MOTOR0_P : motor;
 
 analogWrite(motor, 255);

} 

void stop_motor_sms() {

 MsTimer2::stop();
  
  byte motor = cur_motor;
  
  motor = motor >= 1 ? MOTOR1_P : motor;
  motor = motor == 0 ? MOTOR0_P : motor;
 
 analogWrite(motor, 0);

 motor_ran++;
 
} 

void motor_set_ramp(byte motor, byte ramp) {
    // set motor ramp value, adjust
    // associated values

   
   if( motor > MAX_MOTORS )
     return;
   
   m_ramp_set[motor]   = ramp > 255 ? 255 : ramp;
   
    // calculate speed change per shot  
   if( ramp > 0 ) {
     m_ramp_shift[motor] = (float) m_speeds[motor] / ramp;
     
       // if there's less than one step per jump,
       // we need to skip shots between increases
       // so determine how many shots to skip
      
     if( m_ramp_shift[motor] < 1 ) {
       m_ramp_mod[motor] = ramp / m_speeds[motor];
       m_ramp_mod[motor] = m_ramp_mod[motor] < 2 ? 2 : m_ramp_mod[motor];
       m_ramp_shift[motor] = 1.0;
     }
     else {
       m_ramp_mod[motor] = 0;
     }
   
   }
   else {
     m_ramp_shift[motor] = 0;
     m_ramp_mod[motor]   = 0;
   }
      

}  


void motor_stop_all() {
  // stop all motors
  
      // disable pulsing interrupt if engaged
  if( timer_engaged ) {
    Timer1.detachInterrupt();
    timer_engaged = false;
  }

  digitalWrite(MOTOR0_P, LOW);
  digitalWrite(MOTOR1_P, LOW);
  
  motor_control(0, false);
  motor_control(1, false);
  
  

}


float motor_cal_adjust(byte type, byte motor, byte m_spd, byte dir) {
  
  if( motor > 1 )
    return(1.0);
  
  
      // simplistic for sms mode
  if( type == 0 ) 
    return(m_cal_array[motor][m_angle[motor]][0][dir]);
    
    // determine which calibration position we fall
    // into
    
  byte pos = 0;
  
    // if between two cal points, get position between them
  byte cal_diff = motor_spd_cal[1] - motor_spd_cal[0];
  byte hi_diff  = 255 - motor_spd_cal[1];
  
  if ( m_spd > motor_spd_cal[0] && m_spd < motor_spd_cal[1] ) {
    unsigned int diff = m_spd - motor_spd_cal[0];
    float diff_pct = (float) diff / (float) cal_diff;
    float ret = ( m_cal_array[motor][m_angle[motor]][2][dir] * diff_pct ) + ( m_cal_array[motor][m_angle[motor]][1][dir] * ( 1.0 - diff_pct ) );
    return(ret);
  }
  else if( m_spd > motor_spd_cal[1] ) {
      // between last cal point and max speed
    unsigned int diff = m_spd - motor_spd_cal[1];
    float diff_pct = (float) diff / (float) hi_diff;
    float ret = m_cal_array[motor][m_angle[motor]][2][dir] - (m_cal_array[motor][m_angle[motor]][2][dir] * diff_pct);
    return(ret);
  }
  else if( m_spd <= motor_spd_cal[0] ) {
    pos = 1;
  }
  else if( m_spd == motor_spd_cal[1] ) {
    pos = 2;
  }
  else {
    return(1.0);
  }
  
  return(m_cal_array[motor][m_angle[motor]][pos][dir]);
}
  

void motor_run_pulsing() {
  
    // start pulsing motor movement
  
  if( ! timer_engaged ) {

    // we use timer1, which disables pwm on
    // lcd bkl pin
    if( cur_bkl > 0 ) {
      digitalWrite(LCD_BKL, HIGH);
    }
    else {
      digitalWrite(LCD_BKL, LOW);
    }
      
    digitalWrite(MOTOR0_P, LOW);
    digitalWrite(MOTOR1_P, LOW);
    
    Timer1.initialize(MP_PERIOD);
    Timer1.attachInterrupt(motor_pulse);
    timer_engaged = true;
    timer_used = true;
          
 }
}

void motor_execute_ramp_changes() {
  
      // check for ramping, and ramp up or down as needed
      

  for(byte m = 0; m < MAX_MOTORS; m++) {
      // no ramp, go to next motor
    if( m_ramp_set[m] == 0 )
      continue;
    
      // handle lead-in
    if( shots <= m_lead_in[m] ) {
      motor_set_speed(m, 0); 
      continue;  
    }
      
      // ramp up?
    if( m_ramp_set[m] >= ( shots - m_lead_in[m]) ) {
        // if ramping less than once per shot 
      if( m_ramp_mod[m] > 0 && ( shots - m_lead_in[m] ) % m_ramp_mod[m] == 0 ) {
        motor_set_speed(m, m_speeds[m] + 1);
      }
      else if( m_ramp_mod[m] == 0 ) {  
        motor_set_speed(m, (m_ramp_shift[m] * (shots - m_lead_in[m]) ) );
      }
    }
    else if( (cam_max - shots - m_lead_out[m]) <= m_ramp_set[m] ) {
        // ramping down, it seems
      if( m_ramp_mod[m] > 0 && (cam_max - shots - m_lead_out[m]) % m_ramp_mod[m] == 0 ) {
        byte m_spd = m_speeds[m] > 0 ? m_speeds[m] - 1 : 0;
        motor_set_speed(m, m_spd);
      }
      else if( m_ramp_mod[m] == 0 ) {  
        motor_set_speed(m, m_ramp_shift[m] * (cam_max - shots - m_lead_out[m]) );
      }
    }
  }
      
}


void motor_run_calibrate(byte which, unsigned int mspd, byte dir) {
  

  byte cur_dir = m_wasdir[cur_motor];
  motor_dir(cur_motor, dir);
  
  if( which == 1 ) {
    float m_pct = ( (float) mspd / (float) m_maxsms[cur_motor] );  
    unsigned int run_tm = 60000.0 * m_pct;
    
    motor_ran = 0;
    
    run_motor_sms(cur_motor);
    MsTimer2::set(run_tm, stop_motor_sms);
    MsTimer2::start();
    
    while( ! motor_ran )
      continue;
    
    return;
  }
  else {

       byte was_on_pct = on_pct[cur_motor];
       byte was_off_pct = off_pct[cur_motor];     

       motor_calc_pulse_len(cur_motor, mspd, true);
       
       unsigned long run_tm = millis();
       
       motor_run_pulsing();
       
         // main loop is ~ 10% slower than calibration loop
       while( millis() - run_tm < 58000 ) {
           // introduce timing block delay similar to main loop
         int foo = analogRead(BUT_PIN);
       }

       motor_stop_all();
         
       on_pct[cur_motor]  = was_on_pct;
       off_pct[cur_motor] = was_off_pct;

  }
  
  motor_dir(cur_motor, cur_dir);
  
}


  
  
         
  
