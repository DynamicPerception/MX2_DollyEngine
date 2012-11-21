/*
 * MerlinExample
 *
 * Basic test code for the Arduino MerlinMount library.
 *
 */

#include <merlin_mount.h>

void setup() {
  //start serial used to communicate with Merlin/Orion mount
  Serial.begin(9600);
  
  //Init the Merlin mount and reset start/end position.
  merlin.init();

}



void  loop() {   // run over and over again
	long pos_pitch = 0;
	long pos_yaw = 0;
	
	//Start to move downward
	merlin.startMoving(AXIS_PITCH, DIR_DOWN);
	delay(5000);
	
	//Start to move left as well
	merlin.startMoving(AXIS_PITCH, DIR_LEFT);
	delay(5000);

	//Stop moving
	merlin.stopMoving(AXIS_YAW);
	merlin.stopMoving(AXIS_PITCH);
	
	//Read current position
	pos_pitch = merlin.readAxisPosition(AXIS_PITCH);
	pos_yaw = merlin.readAxisPosition(AXIS_YAW);
	
	pos_yaw = pos_yaw + merlin.fromAngle(20.0);
	
	//Move 20 degrees
	merlin.driveToPositionBothAxis(pos_yaw, pos_pitch, SYNC_SYNCHRONY);
	
	delay(5000);

}
