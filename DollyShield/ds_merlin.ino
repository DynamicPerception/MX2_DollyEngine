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
  Merlin mount control functions
  ========================================
  
*/


//#ifdef MERLIN_ENABLED


void merlin_set_speed(byte axis, float spd) {
    // set speed for axis (axes starting at zero, not 1)
  merlin.init();  
    
    // TODO: convert displayed degrees to values 
    // absolute in merlin
    
 // char resp[16]; - AC:20120114: not needed anymore


  merlin_speeds[axis] = spd;
  // AC:20120115: Set Ratio to 1 before setting speed to ensure correct calculations
  // Ratio 3 is only used in manual move
  merlin.setRatio(axis+1, 1);
  merlin.setSpeed(axis+1, spd);
}

void merlin_send_angle(byte axis, float angle) {
  
  merlin.init();

    // set motor free-running flag
  merlin_flags |= (B10000000 >> axis);
  
   // AC:20120115: Switching to custom drive mode
  // merlin.driveToPosition(axis+1, merlin_dir[axis], angle);
  merlin.moveAngle(axis+1, merlin_dir[axis], angle);
}

byte merlin_running(byte axis) {
  
  merlin.init();
  return merlin.readAxisStatus(axis+1);
}

void merlin_stop(byte axis) {
  merlin.init();
  
    // disable running bit flag
    
  merlin_flags &= (B11111111 ^ (B10000000 >> axis));
  
  merlin.stopMoving(axis+1);
  
}

void merlin_run(byte axis) {
  merlin.init();
  
    // already running
  if( merlin_flags & (B10000000 >> axis) ) 
    return;

    // enable running flag
  merlin_flags |= (B10000000 >> axis);

    // for continuous motion, our speeds are based
    // on 1x ratio
    
    // AC:20120114: removed as ratio is set in set_speed()
 // merlin.setRatio(axis+1, 1); 
  
  merlin.startMoving(axis+1, merlin_dir[axis]);
}

// AC:20120114: added function for manual move in the menu
void merlin_move_manual(byte axis, byte dir) {
  
  // moving at ratio 3 as it works better with higher load (does not slip)
  merlin.init();
  merlin.setRatio(axis+1, 3);
  merlin.setSpeed(axis+1, merlin_man_spd[axis]);
  merlin_set_dir(axis,dir);
  merlin_run(axis);
}


void merlin_set_dir(byte axis, byte dir) {
 
  merlin_wasdir[axis] = merlin_dir[axis];
  merlin_dir[axis]    = dir;
  // AC:20120115: melrin library will call setDirection internally
  // inside startMoving or moveAngle
  // merlin.setDirection(axis+1,dir);
  
}
  



void show_merlin() {
  
 merlin_flags |= B00100000;

 lcd.clear();
 lcd.noBlink();

 lcd.setCursor(0, 0);

 lcd.print("Scope Manual");
 
 lcd.setCursor(0,1);
 
 lcd.print("Enter to Exit");
 
  
}  

void merlin_init() {
    merlin.init();
}


void merlin_run_cont() {

  // run in continuous mode

  if( ! (merlin_flags & B10000000) && merlin_speeds[0] > 0.0 ) {
    merlin_set_dir(0, merlin_dir[0]);
    merlin_run(0);
  }
  
  if( ! (merlin_flags & B01000000) && merlin_speeds[1] > 0.0 ) {
    merlin_set_dir(1, merlin_dir[1]); // dir may not have been set on
                                      // first run if dir not modified in UI
    merlin_run(1);
  }

}

//#endif
