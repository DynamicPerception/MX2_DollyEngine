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
  Alt I/O Handlers
  ========================================
  
*/



void altio_isr_handler(byte which) {
  
    // from internals
  extern volatile unsigned long timer0_millis;
  
  if( timer0_millis - input_trig_last > ALT_TRIG_THRESH ) {
    
    input_trig_last = timer0_millis;
    
    switch( input_type[which] ) {
      
      case 1:
        start_executing();
        break;
        
      case 2:
        stop_executing();
        break;
        
      case 3:
        altio_flip_runstat();
        break;
        
      case 4: 
          // set camera ok to fire
        external_interval |= B00100000;
        break;
        
      case 8:
          // switch all motor directions!
        motor_dir(0, !m_wasdir[0]);
        motor_dir(1, !m_wasdir[1]);
        break;
        
      default:
        break;
    } // end switch
  } //end if timer0...
}

      
void altio_isr_one() {
  altio_isr_handler(0);
}


void altio_isr_two() {
  altio_isr_handler(1);
}


void altio_connect(byte which, byte type) {
  
  input_type[which] = type;

    // type == 5, 6, 7 changes from input to output, handle this
    // deviation
    
  if( type == 5 || type == 6 || type == 7 ) {
          // output mode
    detachInterrupt(which);
    pinMode(2+which,OUTPUT);
      // set correct flag, as needed
    if( type == 5 ) {
      external_trigger |= B10000000 >> which;
    }
    else if( type == 6 ) {
      external_trigger |= B00100000 >> which;
    }
    else {
      external_trigger |= B10100000 >> which;
    }
    
    return;
  }

  if( type == 0 ) {
      detachInterrupt(which);
      digitalWrite(2+which, LOW);
        // disable external interval for this line (just in case it
        // was ever set)
      external_interval &= (B11111111 ^ (B10100000 >> which));
      return;
  }
  else if( type == 4 ) {
    // our external intervalometer functon
    
      // enable external intervalometer for this line
    external_interval |= B10000000 >> which;
  }
  else {
        // disable external interval for this line (just in case it
        // was ever set)
      external_interval &= (B11111111 ^ (B10100000 >> which));
  }
  
    // set pin as input
  pinMode(2+which, INPUT);
    // enable pull-up resistor
  digitalWrite(2+which, HIGH);
  
  if( which ) {
    attachInterrupt(1, altio_isr_two, altio_dir);
  }
  else {
    attachInterrupt(0, altio_isr_one, altio_dir);
  }
  
}    
  
void altio_flip_runstat() {
    // if currently running, stop; if not, start
    
  if( run_status & B10000000 ) {
    // running
    stop_executing();
  }
  else {
    start_executing();
  }
  
}

void alt_ext_trigger_engage(boolean predel) {

  unsigned long dly = predel == true ? ext_trig_pre_delay : ext_trig_pst_delay;
    // set flag
  run_status |= B00001000;
  
    // we use the interrupt pins, 2&3
    
  if( predel == true ) {
    if( external_trigger & B10000000 ) 
      digitalWrite(2, HIGH);
    if( external_trigger & B01000000 )
      digitalWrite(3, HIGH);
  }
  else {
    if( external_trigger & B00100000 ) 
      digitalWrite(2, HIGH);
    if( external_trigger & B00010000 )
      digitalWrite(3, HIGH);
  }        
  
  MsTimer2::set(dly, alt_ext_trigger_disengage);
  MsTimer2::start();
}

void alt_ext_trigger_disengage() {
  
  if( external_trigger & B10100000 )
    digitalWrite(2, LOW);
    
  if( external_trigger & B01010000 )
    digitalWrite(3, LOW);

  MsTimer2::stop();
  
    // clear flag...
  run_status &= B11110111;
}

