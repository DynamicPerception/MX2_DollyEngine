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
      default:
        break;
    }
  }
}

      
void altio_isr_one() {
  altio_isr_handler(0);
}


void altio_isr_two() {
  altio_isr_handler(1);
}


void altio_connect(byte which, byte type) {
  
  input_type[which] = type;
  
  if( type == 0 ) {
      detachInterrupt(which);
      digitalWrite(2+which, LOW);
      return;
  }
  
  
    // set pin as input
  pinMode(2+which, INPUT);
    // enable pull-up resistor
  digitalWrite(2+which, HIGH);
  
  if( which ) {
    attachInterrupt(1, altio_isr_two, FALLING);
  }
  else {
    attachInterrupt(0, altio_isr_one, FALLING);
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


