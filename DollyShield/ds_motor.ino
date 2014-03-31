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
  
  motor_speed_adjust - adjust by val, check against limit
    called only from ds_ui - motor speed setting for manual motion - this is the sole usage
    calls motor_set_speed

  motor_control - turn motor on or off
    called from DollyShield - start/stop executing
    called from ds_ui - motor on/off for manual motion
    calls motor_set_speed to turn motor on/off
  
  motor_set_speed - set actualy motor speed, including stopping motor
    called from ds_ui_values - main screen motor speed adjust

  BUGS: 
  - manual direction changes change non-manual direction
  - manual/normal speeds cross set
  - what is mcur_spds and why is it used? this code is broken
  TODO:
  - save m_speeds but not when ramping (separate var for current motor speed?)
  
*/

// #define debug


void motor_speed_adjust( byte motor, int val, boolean spd_floor ) { // manual control motor speed

   int c_speed = (int) m_speeds[m_mode][motor] + val;  // new speed, possibly <0
    // val is expected to be between -255 and 255  //wbp: 0 and 255 ???

   if( c_speed >= 255 ) {  
    c_speed = 255;
   }
   else {
     c_speed = c_speed >= 0 ? c_speed : 0;
       // do we need to floor the value at the min speed setting? (man control)
     if( spd_floor ) 
       c_speed = c_speed < EE.min_spd[motor] ? EE.min_spd[motor] : c_speed;
   }
   motor_set_speed( motor, c_speed );
   
}

void motor_control(byte motor, boolean state) {

   // turn motors on or off
 
  if( ! state ) {
      // set motors as not running
    unsigned int ths_spd = m_speeds[m_mode][motor];
    motor_set_speed( motor, 0 ); // stop motor
    m_speeds[m_mode][motor] = ths_spd; // restore original motor speed after motor_set_speed
    run_status &= (255-RS_Motors_Running); // motors are stopped
  }
  else {
      // set motors as running...
    run_status |= RS_Motors_Running; // motors are (will be) running
    motor_set_speed(motor, m_speeds[m_mode][motor]);
  }
}



void motor_set_speed( byte motor, unsigned int m_speed ) {
  
  if( motor >= MAX_MOTORS )
    return;
    
  m_speeds[m_mode][motor] = m_speed;
  m_sms_tm[motor] = 0;

  if( EE.motor_mode || (ui_ctrl_flags & (UC_Manual+UC_Park)) ) {
      
      // continuous, manual, or park
      // normalize to max pwm speed
    if (m_speed > 255) {
      m_speed = 255;
      m_speeds[m_mode][motor] = m_speed;
    }
  
  }
  else {

      // interleave mode and not manual control and not parking
    
    float m_pct = ( (float) m_speed / (float) EE.m_maxsms[motor] );  

    m_sms_tm[motor] = 60000.0 * m_pct;
    
    // calibrate
    m_sms_tm[motor] *= motor_cal_adjust(0,motor,0,EE.m_dirs[m_mode][motor]);

  }
  
    // do we need to go into pulsing mode?
  if( m_speed > 0 && m_speed < EE.min_spd[motor]  ) {
    motor_calc_pulse_len(motor, m_speed, false);
  } //      
  else {
    on_pct[motor] = 0;
  }
  
  byte motor_pin = motor >= 1 ? MOTOR1_P : MOTOR0_P;
  
  if( ! (run_status & RS_Motors_Running)  ) {
    // if stopped, do not move motor, just set speed & return
//    m_speeds[m_mode][motor] = m_speed;  //wbp: already done
    return;
  }
  else if( ! (ui_ctrl_flags & (UC_Manual+UC_Park)) && m_sms_tm[motor] > 0 ) {
      // just in case
    digitalWrite(motor_pin, LOW);
      // return if we're in an SMS condition
      // and not in manual mode
    return;
  }

    // if we've made it this far, set motor pin
    // to given analog speed

    // only set analog speed if it exceeds min speed

  if( m_speed >= EE.min_spd[motor] ) {
    analogWrite(motor_pin, m_speed);
    m_state[motor] = 1; // motor is moving
  }
  else {
    // just in case... switching down from 
    // pwm to pulsed...
    digitalWrite(motor_pin,LOW);
    m_state[motor] = 0; // motor is stopped
  }
  
}

void motor_calc_pulse_len(byte motor, unsigned int m_speed, boolean ignore_cal) {
    // for how many periods should the motor be on and
    // off? (pulsing mode)     

    float m_pct   = ( (float) m_speed / 255.0 );
    float periods = 1333.0; // 25 times a second 
    
    on_pct[motor] = periods * m_pct; 
    off_pct[motor] = (float) (periods - on_pct[motor]);
      
    float cal_amt = motor_cal_adjust(1,motor,m_speed, EE.m_dirs[m_mode][motor]);
    
      // calibrate, if desired
    if( ! ignore_cal && cal_amt != 1.0 )
      off_pct[motor] = ( (double) off_pct[motor] * ( cal_amt * EECal.m_cal_constant[motor] )  ); 
        
    if(on_pct[motor] < 1)
      on_pct[motor] = 1;

    // make sure that we're on for a minimum amount of time
    
   if ( on_pct[motor] != 0 && on_pct[motor] < EE.m_min_pulse[motor] ) {
       // adjust so that off time is increased relative to on time
     float diff = (float) EE.m_min_pulse[motor] / (float) on_pct[motor];       
     off_pct[motor] = ((float) off_pct[motor] * diff);
     on_pct[motor] = EE.m_min_pulse[motor];
   }
 
}

void motor_dir( byte motor, byte dir ) {
  
  byte m_dir = EE.m_dirs[m_mode][motor]; // current saved direction for this motor
  if( m_dir != dir ) { // is it changing?
    EE.m_dirs[m_mode][motor] = dir; // remember new direction
    ee_save(); // save it
//    return;  // no change in direction needed - except it might never have been set...
  }

  if (EE.ui_invdir) // if invert dir on
    dir = !dir; // flip direction 

   // get current speed for the motor
  unsigned int m_speed = m_speeds[m_mode][motor];
//    if( m_speed > 0 ) { // shouldn't this be checking something else?
    if( run_status & RS_Motors_Running) {
        // motor was already moving, need to stop it first
      motor_set_speed( motor, 0 );
      delay(200);  // give motor a little time to stop
    }
    
    // find direction pin
  byte m_dirp = motor >= 1 ? MOTOR1_DIR : MOTOR0_DIR;

    // reverse direction of motor #1, as the hardware
    // has the directions reversed.
    
  byte m_dirc = motor >= 1 ? !dir : dir;
  
  digitalWrite(m_dirp, m_dirc);  // change the pin
  
  if( run_status & RS_Motors_Running)
    motor_set_speed( motor, m_speed );
}
  

float motor_calc_ipm(byte motor, unsigned int spd, boolean ths_mode) {
  // calculate ipm for a given speed and mode

    // get max speed for either pulse or sms mode
    // on calibration screen, always ch

  float maxspd = ( ! ths_mode ) ? (float) EE.m_maxsms[motor] : 255.0;
  
    // in manual mode, we're always in 0-255 mode
  if( ui_ctrl_flags & (UC_Manual+UC_Park) ) 
    maxspd = 255.0;
    
  float cur_ipm = (float) EE.max_ipm[motor] * ( (float) spd / (float) maxspd );      
  
  return(cur_ipm);
}


void motor_update_dist(byte motor, float rpm, float diarev ) {
    // set distance settings when rpm or diarev change
    
  EE.max_ipm[motor] = rpm * diarev;
  EE.min_spd[motor] = 255 * ( EE.min_ipm[motor] / EE.max_ipm[motor] );
  EE.m_maxsms[motor] = EE.max_ipm[motor] * 100;
  
	ee_save();

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
            m_state[i] = 1; // wbp
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
            m_state[i] = 0; // wbp
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

  cur_motor = motor;  // save for stop_motor_sms
//  motor = motor >= 1 ? MOTOR1_P : motor;
//  motor = motor == 0 ? MOTOR0_P : motor;
  byte mpin = MOTOR0_P;
  if (motor > 0)
    mpin = MOTOR1_P;
 
 analogWrite(mpin, 255);
 m_state[motor] = 1; // wbp motor is running

} 

void stop_motor_sms() {

 MsTimer2::stop();
  
//  motor = motor >= 1 ? MOTOR1_P : motor;
//  motor = motor == 0 ? MOTOR0_P : motor;
  byte mpin = MOTOR0_P;
  if (cur_motor > 0)
    mpin = MOTOR1_P;
 
 analogWrite(mpin, 0);

 motor_ran++;
 m_state[cur_motor] = 0; // wbp motor is stopped
 
} 

void motor_set_ramp(byte motor, byte ramp) {
    // set motor ramp value, adjust associated values
   
   if( motor > MAX_MOTORS )
     return;
   
   ramp = ramp > 255 ? 255 : ramp; // limit 255
   EE.m_ramp_set[motor] = ramp; 

//Serial.print("set ramp: set "); Serial.print(EE.m_ramp_set[motor]);
//Serial.print(", m_speed "); Serial.print(m_speeds[m_mode][motor]);
   
    // calculate speed change per shot  
   if( ramp > 0 ) {
     m_ramp_shift[motor] = (float) m_speeds[m_mode][motor] / (float) ramp;
   }
   else {
     m_ramp_shift[motor] = 0;
   }

//Serial.print(", shift "); Serial.print(m_ramp_shift[motor],2);
//Serial.print(", speed "); Serial.print(m_ramp_speed[motor]);
//Serial.println("");

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
    return(EECal.m_cal_array[motor][EE.m_angle[motor]][0][dir]);
    
    // determine which calibration position we fall into
    
  byte pos = 0;
  
    // if between two cal points, get position between them
  byte cal_diff = EECal.m_cal_speed[1] - EECal.m_cal_speed[0];
  byte hi_diff  = 255 - EECal.m_cal_speed[1];
  
  if ( m_spd > EECal.m_cal_speed[0] && m_spd < EECal.m_cal_speed[1] ) {
    unsigned int diff = m_spd - EECal.m_cal_speed[0];
    float diff_pct = (float) diff / (float) cal_diff;
    float ret = ( EECal.m_cal_array[motor][EE.m_angle[motor]][2][dir] * diff_pct ) + ( EECal.m_cal_array[motor][EE.m_angle[motor]][1][dir] * ( 1.0 - diff_pct ) );
    return(ret);
  }
  else if( m_spd > EECal.m_cal_speed[1] ) {
      // between last cal point and max speed
    unsigned int diff = m_spd - EECal.m_cal_speed[1];
    float diff_pct = (float) diff / (float) hi_diff;
    float ret = EECal.m_cal_array[motor][EE.m_angle[motor]][2][dir] - (EECal.m_cal_array[motor][EE.m_angle[motor]][2][dir] * diff_pct);
    return(ret);
  }
  else if( m_spd <= EECal.m_cal_speed[0] ) {
    pos = 1;
  }
  else if( m_spd == EECal.m_cal_speed[1] ) {
    pos = 2;
  }
  else {
    return(1.0);
  }
  
  return(EECal.m_cal_array[motor][EE.m_angle[motor]][pos][dir]);
}
  

void motor_run_pulsing() {
  
    // start pulsing motor movement
  
  if( ! timer_engaged ) {

    // we use timer1, which disables pwm on
    // lcd bkl pin
    if( EE.cur_bkl > 0 ) {
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
    if( EE.m_ramp_set[m] == 0 )
      continue;
    
      // handle lead-in
    if( shots <= EE.m_lead_in[m] ) {
      motor_set_speed(m, 0); 
      continue;  
    }

		float ramp_speed = 0;
      // ramp up?
    if( EE.m_ramp_set[m] >= ( shots - EE.m_lead_in[m]) ) {
      ramp_speed = m_ramp_shift[m] * (shots - EE.m_lead_in[m]); // calc new speed (float)
			if (ramp_speed > EE.m_speeds[m_mode][m]) // probably not necessary but better safe than ...
			  ramp_speed = EE.m_speeds[m_mode][m];
      motor_set_speed(m, int(ramp_speed+0.5)); // set new motor speed
    }
    else if( (EE.cam_max - shots - EE.m_lead_out[m]) <= EE.m_ramp_set[m] ) {
        // ramping down, it seems
      ramp_speed =  m_ramp_shift[m] * (EE.cam_max - shots - EE.m_lead_out[m]); // calc new speed (float)
			if (ramp_speed < 0) // probably not necessary but better safe than ...
			  ramp_speed = 0;
      motor_set_speed(m, int(ramp_speed+0.5)); // set new motor speed
    }
  }
      
}


void motor_run_calibrate(byte which, unsigned int mspd, byte dir) {
  

  byte cur_dir = EE.m_dirs[m_mode][cur_motor];
  motor_dir(cur_motor, dir);
  
  if( which == 1 ) {
    float m_pct = ( (float) mspd / (float) EE.m_maxsms[cur_motor] );  
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

