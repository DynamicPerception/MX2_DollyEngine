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


void altio_connect(byte which, byte type) {

		// for type == Out Before, Out After, Out Both pin changes from input to output, 
		// handle this deviation
	if( type == IT_OutBefore || type == IT_OutAfter || type == IT_OutBoth ) {
			// output mode
		pinMode(2+which,OUTPUT);
//	  Serial.print("pinMode ");	Serial.print(2+which, DEC); Serial.println(" OUTPUT");
			// set correct flag, as needed
		if( type == IT_OutBefore ) {
			external_trigger |= B10000000 >> which;
		}
		else if( type == IT_OutAfter ) {
			external_trigger |= B00100000 >> which;
		}
		else {
			external_trigger |= B10100000 >> which;
		}
		return;
	}

	if( type == IT_Disabled ) {
			digitalWrite(2+which, LOW);
				// disable external interval for this line (just in case it
				// was ever set)
			external_interval &= (B11111111 ^ (B10100000 >> which));
			return;
	}
	
	if( type == IT_ExtInterval ) {
		// our external intervalometer functon
		
			// enable external intervalometer for this line
		external_interval |= B10000000 >> which;
	}
	else {
				// disable external interval for this line (just in case it
				// was ever set)
			external_interval &= (B11111111 ^ (B10100000 >> which));
	}
		
		// set pin as input & enable pull-up resistor
	pinMode(EXT_PIN1+which, INPUT_PULLUP);
//  Serial.print("pinMode ");	Serial.print(2+which, DEC); Serial.println(" INPUT_PULLUP");
	
}    
  
void altio_flip_runstat() {
    // if currently running, stop; if not, start
    
  if( run_status & RS_Running ) {
    // running
    stop_executing();
  }
  else {
    start_executing();
  }
  
}

void alt_ext_trigger_engage(boolean predel) {

  unsigned long dly = predel == true ? EE.ext_trig_pre_delay : EE.ext_trig_pst_delay;
    // set flag
  run_status |= RS_External_Trigger;
  
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
  run_status &= (255-RS_External_Trigger);
}
