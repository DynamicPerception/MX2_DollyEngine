/*
  merlin_mount.cpp - Arduino library support for Merlin/Orion telescope mounts
  Copyright (c)2009 Gustav Evertsson All right reserved
  www.guzzzt.com - me@guzzzt.com
 
	Changes by C.A. Church for openmoco.org 11/1/2010
  
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
	
  Version:   1.0 - Aug 7 2009
  Version:	 2.0 - Nov 1 2010
	
*/

#include <inttypes.h>
#include "HardwareSerial.h"

#ifndef	MERLIN_MOUNT_H
#define MERLIN_MOUNT_H

#define INIT_SUCCESS 1
#define INIT_FAILED  0

#define AXIS_YAW   1
#define AXIS_PITCH 2

#define DIR_UP    0
#define DIR_DOWN  1
#define DIR_RIGHT 0
#define DIR_LEFT  1
#define DIR_STOPPED 2

#define STATUS_STOPPED 0
#define STATUS_RUNNING 1

#define SYNC_ASYNCHRONY 0
#define SYNC_SYNCHRONY  1


class MerlinMount
{
	private:
		
		uint8_t 			_initiated;
		uint8_t 			_ratio[2];
		char 					_lastCmd[16];
		long    			_precision[2];
		unsigned long _curSpd[2];
		unsigned long _sidereal[2];
		unsigned int	_movePrecisionSteps;
		
		void _getResponse(char* response);
		void _printHex(unsigned long speed);
		
		
  public:

			// constructor
	  MerlinMount();
		
	  	// Init mount
	  uint8_t init();

		
			// Parse/convert values
	  long parsePosVal(char* hexStr);
	  void valToHexStr(long val, char* hexStr);
	  double toAngle(long first, long second);
	  long fromAngle(double angle);

	  	// set speed ratio
	  uint8_t setRatio(uint8_t axis, uint8_t ratio);
	  	// get step count for every 360' (precision)
	  long getSteps(uint8_t axis);

			// set direction command
	  void setDirection(uint8_t axis, uint8_t dir);
			// get sidereal speed rate
	  long getSidereal(uint8_t axis);

		void setMovePrecision(unsigned int threshold);	  

			// Motor Control functions	
    void startMoving(uint8_t axis, uint8_t dir, unsigned long spd);
    void startMoving(uint8_t axis, uint8_t dir);
    void stopMoving(uint8_t axis);
    void runMotor(uint8_t axis, char* res);
    void moveAngle(uint8_t axis, uint8_t dir, float angle);
	void moveToPrecise(uint8_t axis, unsigned long speed, long position);
    void driveToPosition(uint8_t axis, long position);
    void driveToPositionBothAxis(long positionYaw, long positionPitch, uint8_t sync);
		
			// set speed value
    void setSpeed(uint8_t axis, unsigned long spd);
		void setSpeed(uint8_t axis, float degrees);
		
  // Camera functions
  	void takePicture(int shutterTime);
  	
	// Status functions
		long readAxisPosition(uint8_t axis);
		uint8_t readAxisStatus(uint8_t axis);
	
	// Communication with the Merlin mount
		void sendCommand(const char* send, char* respose);
};

extern MerlinMount merlin;
#endif
