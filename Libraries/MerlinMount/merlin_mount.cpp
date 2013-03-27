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
  
*/

#include "Arduino.h"		//all things wiring / arduino

#include "merlin_mount.h"
#include "HardwareSerial.h"


  // Init
uint8_t MerlinMount::init() {
	char res[16];
	
	if(this->_initiated == 1)
		return INIT_SUCCESS;

	// cchurch - changes
	
		// check to make sure motors are present..
	this->sendCommand(":F1\r", res);
	this->sendCommand(":F2\r", res);	

		// get information about both axes
	_precision[0] = this->getSteps(AXIS_YAW);	
	_sidereal[0]  = this->getSidereal(AXIS_YAW);

	_precision[1] = this->getSteps(AXIS_PITCH);	
	_sidereal[1]  = this->getSidereal(AXIS_PITCH);
	
	
	this->_initiated = 1;

	return INIT_SUCCESS;
}

  // Control functions
void MerlinMount::startMoving(uint8_t axis, uint8_t dir, unsigned long spd) {

	// modified cchurch - more control over motor movement
	
	char res[16];

	  // stop any current movement
	this->stopMoving(axis);
	
	  // set direction and speed ratio
	  // (speed ratio uses setRatio to set value)
	this->setDirection(axis, dir);
	
	  // set speed value	
	this->setSpeed(axis, spd);
	
	  // tell motor to start moving
	this->runMotor(axis, res);

	
}

void MerlinMount::startMoving( uint8_t axis, uint8_t dir ) {
	 // call without speed to use saved speed
	this->startMoving(axis, dir, _curSpd[axis - 1]);
}

void MerlinMount::runMotor(uint8_t axis, char* res) {
		// turn the motor on
	Serial.print(":J");
	Serial.print(axis, DEC);	
	this->sendCommand("\r", res);
}

void MerlinMount::stopMoving(uint8_t axis) {
	char res[16];

	Serial.print(":L");
	Serial.print(axis, DEC);
	this->sendCommand("\r", res);
}

long MerlinMount::getSidereal(uint8_t axis) {
		// cchurch - wrap up get sidereal command
		// for re-use
	char res[16];

	Serial.print(":D");
	Serial.print(axis,DEC);
	this->sendCommand("\r", res);
	
	return this->parsePosVal(res); 
}


void MerlinMount::setDirection(uint8_t axis, uint8_t dir) {
		// cchurch - wrap up set direction/ratio command
		// for re-use
	char res[16];

	Serial.print(":G");
	Serial.print(axis,DEC);
	Serial.print(_ratio[axis - 1], DEC);
	Serial.print(dir, DEC);

	this->sendCommand("\r", res);
}

void MerlinMount::setMovePrecision(unsigned int threshold) {
	
	_movePrecisionSteps = threshold;
}

	// move drive a specified angle by running a motor and reading its position
	// call setMovePrecision() to set desired accuracy in encoder steps
	// the faster you move, the higher this must be set!
void MerlinMount::moveToPrecise(uint8_t axis, unsigned long speed, long position) {

	long tempPos = this->readAxisPosition(axis);
	if(tempPos == position)
		return;
	
	uint8_t dir = tempPos > position;	

	uint8_t oldRatio = this->setRatio(axis, 1);	
	this->setDirection(axis, dir);	
		
	this->startMoving(axis, dir, speed);

		// loop, reading value and getting position back
		// note that the threshold is also your minimum accuracy
		// amount.
	
	while( 1 ) {
		tempPos = this->readAxisPosition(axis);
		
			// if we hit our target, break out of loop
		if( tempPos >= (position - _movePrecisionSteps) && tempPos <= (position + _movePrecisionSteps) ) 
			break;		
	}			
	this->stopMoving(axis);
//	Serial.println(tempPos);
	this->setRatio(axis, oldRatio);

	return;
}



	// move drive a specified angle using a single GOTO command
void MerlinMount::moveAngle(uint8_t axis, uint8_t dir, float angle) {
	

	long stepsAngle = (angle * ((float) _precision[axis - 1] / 360.0 ));

	long targetPos = this->readAxisPosition(axis);
	
		// determine where we want to land by which 
		// dir we are heading and number of steps to undershoot
		// in order to prevent backlash when reversing gears
	if( ! dir ) {	
		targetPos = targetPos + stepsAngle;
	}
	else {
		targetPos = targetPos - stepsAngle;
	}			
	this->driveToPosition(axis, targetPos);
}


// move drive a specified angle using a single GOTO command
void MerlinMount::driveToPosition(uint8_t axis, long position) {
	char res[16];	
	long tempPos = 0;
	
	tempPos = this->readAxisPosition(axis);
	if(tempPos == position)
		return;
	
	
	uint8_t dir = tempPos > position;	

	uint8_t oldRatio = this->setRatio(axis, 4);
	this->setDirection(axis, dir);
	
	Serial.print(":S");
	Serial.print(axis, DEC);
	this->_printHex(position);
	this->sendCommand("\r", res);
	
	this->runMotor(axis, res);

	this->setRatio(axis, oldRatio);
	
}


void MerlinMount::driveToPositionBothAxis(long positionYaw, long positionPitch, uint8_t sync) {
	driveToPosition(AXIS_YAW, positionYaw);
	driveToPosition(AXIS_PITCH, positionPitch);
	
	// if synchrony then wait unit both axis have stopped
	if(sync == SYNC_SYNCHRONY) {
		delay(600);
		while(this->readAxisStatus(AXIS_YAW) == STATUS_RUNNING ||
					this->readAxisStatus(AXIS_PITCH) == STATUS_RUNNING) {
			delay(200);
		}
	}

}

  // Camera functions
void MerlinMount::takePicture(int shutterTime) {
	char res[10];
	
	this->sendCommand(":O11\r", res);
	delay(shutterTime);
	this->sendCommand(":O10\r", res);
}

  	
	// Status functions
long MerlinMount::readAxisPosition(uint8_t axis) {
	char res[16];

	Serial.print(":j");
	Serial.print(axis, DEC);
	this->sendCommand("\r", res);
	
	return this->parsePosVal(res);
}

uint8_t MerlinMount::readAxisStatus(uint8_t axis) {
	char res[16];

	Serial.print(":f");
	Serial.print(axis, DEC);
	this->sendCommand("\r", res);
	
	if(res[1] == '0')
		return STATUS_STOPPED;
	else
		return STATUS_RUNNING;
}

long MerlinMount::getSteps(uint8_t axis) {

	// cchurch - get number of pulses/steps in a 
	// complete circle
	
	char res[16];
	
	Serial.print(":a");
	Serial.print(axis, DEC);
	this->sendCommand("\r", res);
	
	return this->parsePosVal(res);
	
}

uint8_t MerlinMount::setRatio(uint8_t axis, uint8_t ratio) {
	uint8_t oldRatio = _ratio[axis - 1];
	_ratio[axis - 1] = ratio;
	return oldRatio;
}

void MerlinMount::_printHex(unsigned long val) {

	// send hex value in little-endian form,
	// with nibbles in original order
	
    for( byte i = 0; i < 3; i++) {
  	  byte x = (byte) ( val >> (8*i)  );

	// deal with stupidity in Serial.print(v, HEX);	
          
          if( x == 0 ) {
            Serial.print("00");
          }
          else if( x > 0xF ) {
            Serial.print(x, HEX);
          }
          else {
            Serial.print('0');
            Serial.print(x, HEX);
          }
         
  }
}



	// Low byte is sent first (A35483 is read 8354A3).
long MerlinMount::parsePosVal(char* hexStr) {
	long val = 0;
	char c = 0;
  for(int i = 5; i > 0; i = i - 2) {
    val <<= 4;
    c = hexStr[i-1];
      
    if( c >= '0' && c <= '9')
      val += (c - '0');
    else if( c >= 'A' && c <= 'F')
      val += (c - 'A' + 10);


    val <<= 4;
    c = hexStr[i];
      
    if( c >= '0' && c <= '9')
      val += (c - '0');
    else if( c >= 'A' && c <= 'F')
      val += (c - 'A' + 10);
  }
   return val;
}

void MerlinMount::valToHexStr(long val, char* hexStr) {

		// cchurch - this fails on some speeds, use
		// _printHex for speeds/distances instead
		
	char hexval[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	const byte* p = (const byte*)(const void*)&val;
	
	hexStr[0] = hexval[((*p >> 4) & 0xF)];
	hexStr[1] = hexval[*p & 0x0F];
	p++;
	hexStr[2] = hexval[((*p >> 4) & 0xF)];
	hexStr[3] = hexval[*p & 0x0F];
	p++;
	hexStr[4] = hexval[((*p >> 4) & 0xF)];
	hexStr[5] = hexval[*p & 0x0F];
}

double MerlinMount::toAngle(long first, long second) {
	long distance = 0;
	distance = first - second;
	
	return (double)distance / (double) _precision[0] * 360.0;
}

long MerlinMount::fromAngle(double angle) {
	return (long)(((double)_precision[0] / 360.0) * angle);
}

/* Modified 10/26/2010
   cchurch : add support for variable speeds
*/

	// Communication with the Merlin mount
void MerlinMount::sendCommand(const char* send, char* response) {
  
  Serial.print(send);
			
  delay(50);
  this->_getResponse(response);
  
}



void MerlinMount::setSpeed(uint8_t axis, unsigned long speed) {
	
  char response[16];
  
  Serial.print(":I");
  Serial.print(axis, DEC);
    
  this->_printHex(speed);
  Serial.print("\r");
  delay(50);
  this->_getResponse(response);
  
  _curSpd[axis - 1] = speed;
	
	
}


	
void MerlinMount::setSpeed( uint8_t axis, float degrees ) {
		// set speed value based on degrees per min (rather than integer #)
	
	double interim = 360.0 / (float) _precision[axis - 1];
	interim /= degrees;
	interim *= 60.0;
	
	double speed = 0;
	
	if( _ratio[axis - 1] == 1 ) {
		speed = 19531.25 * interim;
	}
	else {
		speed = 666666.0 * interim;
	}
	
	if( speed > int(speed) ) {
		speed = int(speed) + 1;
	}
	
	this->setSpeed(axis, (unsigned long) speed);
}
		

void MerlinMount::_getResponse(char* response) {
		// handle reading response back from telescope head
  uint8_t incomingByte = 0;
  uint8_t pos = 0;
  uint8_t found = 0;
	
  while( Serial.available() == 0 ) { ; }
	
  while (Serial.available() > 0) {
		
    // read the incoming byte:
    incomingByte = Serial.read();
		
			// ignore everything until we read a valid
			// response start char from the telescope head...
		if( incomingByte == '=' || incomingByte == '!' ) {
			found = 1;
				// do not include response start char in return value
			continue;
		}
		
	  if( found == 1 ) 
			response[pos++] = incomingByte;
		
  }
	
  response[pos] = '\0';
}

  // class constructor
MerlinMount::MerlinMount() {
	// set default values
	// AC/20120113: changed default ratios to 1
	// beacause that is what we always use except in goto
  this->_ratio[0] = 1;
  this->_ratio[1] = 1;

	this->_precision[0] = 0;
	this->_precision[1] = 0;

  this->_sidereal[0]  = 0;
  this->_sidereal[1]  = 0;
	
	this->_curSpd[0] = 0;
	this->_curSpd[1] = 0;
		
	this->_movePrecisionSteps = 100;
	
  this->_initiated = 0;
}




MerlinMount merlin = MerlinMount();

