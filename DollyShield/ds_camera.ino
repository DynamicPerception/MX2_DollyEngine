/*


OpenMoco
 - Time-lapse Core Engine

 - Modified for DollyShield (MX2) 6/2010 changes by cchurch/dynamicperception
 
  
  See www.openmoco.org for more information



    (c) 2008-2010 C.A. Church

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
  Camera control functions
  ========================================
  
*/

void fire_camera(unsigned long exp_tm) {

   
    // determine if focus pin should be brought high
    // w. the shutter pin (for some nikons, etc.)
    
  if( EE.focus_shutter )
    digitalWrite(FOCUS_PIN, HIGH);
    
  digitalWrite(CAMERA_PIN, HIGH);
    // start timer to stop camera exposure
  MsTimer2::set(exp_tm, stop_camera);
  MsTimer2::start();

    // update camera currently engaged
    // (turn on bit)
  run_status |= RS_Camera_Active;
  
  return;
}


void stop_camera() {
  
  digitalWrite(CAMERA_PIN, LOW);
 
    // we do this every time, because
    // it's possible that the flag
    // that controls whether or not to
    // trip focus w. shutter may have
    // been reset during our exposure,
    // and failing to do so would keep
    // the focus pin high for a long
    // time.
    
  digitalWrite(FOCUS_PIN, LOW);
 
    // turn off timer - we do this
    // after the digitalWrite() to minimize
    // over-shooting in case this takes some
    // unusually-long amount of time
    
  MsTimer2::stop();

    // are we supposed to delay before allowing
    // the motors to move?  Register a timer
    // to clear out status flags, otherwise
    // just clear them out now.
    
    // the delay is used to prevent motor movement
    // when shot timing is controlled by the camera.
    // the post-delay should be set to an amount greater
    // than the max possible camera exposure timing
    
      // update camera currently engaged
  run_status &= (255-RS_Camera_Active);

      // update camera cycle complete
  run_status |= RS_Camera_Complete;
   
}


void camera_clear() {
  // clears out camera engaged settings
  // so that motor control and other actions can 
  // be undertaken.  Used as a timer whenever
  // a camera post delay is set.
  
 MsTimer2::stop(); // turn off timer
 
      // update camera currently engaged
 run_status &= (255-RS_Camera_Active);

      // update camera cycle complete
 run_status |= RS_Camera_Complete;
 
}  



void stop_cam_focus() {
  
  MsTimer2::stop();
  digitalWrite(FOCUS_PIN, LOW);
  pre_focus_clear = 2;
  
}

void clear_cam_focus() {
  MsTimer2::stop();
  pre_focus_clear = 4;
}

float calc_total_cam_tm() {
 
     // calculate total minimum time between exposures 
     
  byte pf_tm = 0;
  
    // add 100ms pre-focus tap clear value
  if( EE.focus_tap_tm > 0 ) 
    pf_tm = EE.focus_tap_tm + 100;
    
  float total = (float) ( EE.exp_tm + pf_tm + EE.post_delay_tm  );
  
  if( ! EE.motor_mode )
    total += m_sms_tm[0] + m_sms_tm[1];
    
  total = total / 1000.00;

  return(total);
}
