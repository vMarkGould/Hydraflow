// This #include statement was automatically added by the Particle IDE.
#include "TM1637Display/TM1637Display.h"

// This #include statement was automatically added by the Particle IDE.
#include "HttpClient/HttpClient.h"

#include <stdio.h>
#include <math.h>

//Button from Grove starter Kit
#define BUTTONPIN D6
//Used for 4 digit display from Grove starter Kit
#define CLK D4
//Used for 4 digit display from Grove starter Kit
#define DIO D5
//rotary angle sensor from Grove starter Kit
#define waterCapacity A0

//Interupt used to monitor when the capacity of water on the Angle Sensor is changing
byte waterCapacityInterrupt = A0;

//used to display water consumed on the 4 digit display
void dispWaterConsumed(unsigned int num);
TM1637Display display(CLK,DIO);


//Interupt used to monitor when flow is occuring on the water flow meter
byte waterSensorInterrupt = D2;  
byte waterSensorPin       = D2;

// The hall-effect flow sensor outputs approximately 42 pulses per second per
// litre/minute of flow.
float calibrationFactor = 42;

//Calibrates the rotary angle sensor to Milliletters
float rotationRotaToMilliLetres = 2.44;

//Count Pulses from flow sensor, and measure capacity change from rotary angle sensor
volatile byte waterPulseCount,capacityChange;


float waterFlowRate; //used to track water flow rate for duration of interupt
unsigned int  waterFlowMilliLitres; //measures the amount of water flowed for that time period in the interupt
unsigned long  waterTotalMilliLitres; //measures the total amount of water flowed in that interupt
//how much water is in the hydration blader, water remaining as a percentage, amount of water consumed since reset, used to send to Particle Cloud as event when capacity changes, Add the water consumed during that interupt to the total water consumed//
int milliLitresCarried, waterPercent, milliLitresConsumed, capacityChangeINT,  milliLitresSaved;
//Used for conversion to decimal for calculations 
float milliLitresCarriedDec, waterPercentDec, milliLitresConsumedDec;
boolean  waterFlag; //used to trigger adding up water during the interupt

// used to track various timers
unsigned long oldTime, oldTimeMaster, showTime;
int addr = 2; //address of EEPROM to use to store water consumed
String resettest="reset"; //string expected when reset is triggered

//Future use for collecting full history from device of last number of resets
//char history[3000];

//Variable to track if the amount of water in each second of the interupt should be published
bool publishGulps = true;

//Deffines my LED Constants for the various values for Percentage of water remaining
    const uint8_t SEG_90[] = {
	SEG_E | SEG_F | SEG_B | SEG_C,  // ||
	SEG_E | SEG_F | SEG_B | SEG_C,  // ||
	SEG_E | SEG_F | SEG_B | SEG_C,  // ||
	SEG_E | SEG_F | SEG_B | SEG_C   // ||
	};
	

    const uint8_t SEG_80[] = {
	SEG_B | SEG_C,                  // |
	SEG_E | SEG_F | SEG_B | SEG_C,  // ||
	SEG_E | SEG_F | SEG_B | SEG_C,  // ||
	SEG_E | SEG_F | SEG_B | SEG_C   // ||
	};
	

    const uint8_t SEG_70[] = {
	SEG_G,
	SEG_E | SEG_F | SEG_B | SEG_C,  // ||
	SEG_E | SEG_F | SEG_B | SEG_C,  // ||
	SEG_E | SEG_F | SEG_B | SEG_C   // ||
	};

    const uint8_t SEG_60[] = {
	SEG_G,
	SEG_B | SEG_C,                  // |
	SEG_E | SEG_F | SEG_B | SEG_C,  // ||
	SEG_E | SEG_F | SEG_B | SEG_C   // ||
	};
	
    const uint8_t SEG_50[] = {
	SEG_G,
	SEG_G,
	SEG_E | SEG_F | SEG_B | SEG_C,  // ||
	SEG_E | SEG_F | SEG_B | SEG_C  // ||
	};
	
	const uint8_t SEG_40[] = {
	SEG_G,
	SEG_G,
	SEG_B | SEG_C,                 // |
	SEG_E | SEG_F | SEG_B | SEG_C  // ||	
	};

	const uint8_t SEG_30[] = {
    SEG_G,
    SEG_G,
	SEG_G,
	SEG_E | SEG_F | SEG_B | SEG_C  // ||
	};
	
    const uint8_t SEG_20[] = {
    SEG_G,
    SEG_G,
	SEG_G,
    SEG_B | SEG_C                 // |
	};
	
    const uint8_t SEG_LO[] = {
	SEG_D | SEG_E | SEG_F,                           // L
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
	};
	
	const uint8_t SEG_OUT[] = {
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,    // O
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,           // U
	SEG_D | SEG_E | SEG_F | SEG_G,                   // t
	};

void setup() {
	//Publish to pack labs tracking event cue so that Paclabs can see when we are working in projects
	Spark.publish("notifyr/announce", "Gouldies Photon is up and running! HydraFlow - are you parched?");
	Spark.publish("hydraflow/version", "Gouldies Photon is up and running! HydraFlow version 2.0");
	// Initialize a serial connection for reporting values to the host
	Serial.begin(38400);


	//Set the Pin For the reset button
	pinMode(BUTTONPIN, INPUT);
	//set brightness
	display.setBrightness(0x0f);
	
	//Set water flow sensor Pin to input
	pinMode(waterSensorPin, INPUT);
	//keep voltage high so we can detect changes
	digitalWrite(waterSensorPin, HIGH);

	//keeps track of water flowing and perform action when there is water in the total or water is still flowing
	waterFlag = false;
	waterPulseCount, capacityChange = 0;
	capacityChangeINT = 0;
	waterFlowRate = 0.0;
	waterFlowMilliLitres = 0;
	waterTotalMilliLitres = 0;
	milliLitresConsumed =0;
	milliLitresSaved = 0;
	milliLitresCarried = 0;
	oldTime = 0;
	oldTimeMaster = 0;
	waterPercent = 100;

	EEPROM.get(addr, milliLitresConsumed);
	Serial.println(milliLitresConsumed);
	//int flow = 100;
	//int histtime = 5;
	//char flowHistory[3000];
	//sprintf(flowHistory, "{\"drunk\":%d,\"time\":%d}", flow, histtime); 	
	//strcpy (history, flowHistory);    

	//initialise the display
	dispWaterConsumed(milliLitresConsumed); //Display Water Consumed on Display
	//grab the carried water from the rotary Angle sensor
	milliLitresCarried = analogRead(waterCapacity)*rotationRotaToMilliLetres;
  
	//Future use for History
	//int consumedEEprom = 0;
	//EEPROM.get(addr, consumedEEprom);
    //milliLitresConsumed = consumedEEprom;
    
	
	//Define Particle Variables for use in Web Page
	//Particle.variable("History", history, STRING);
	Spark.variable("Carried", &milliLitresCarried, INT);
	Spark.variable("TotalDrunk", &milliLitresConsumed, INT);
	Spark.variable("Percent", &waterPercent, INT);

  

	//define particle function for reseting the consumption level from API
	Spark.function("resetMlDrunk",resetCounter);

	// The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
	// Configured to trigger on a FALLING state change (transition from HIGH
	// state to LOW state)
	attachInterrupt(waterSensorInterrupt, waterPulseCounter, FALLING);

}

/**
 * Main program loop
 */
void loop() {

    if((digitalRead(BUTTONPIN) == 1) && (milliLitresCarried==0)){
         resetCounter(resettest);

      //  Spark.publish("Liters_Consumed_reset", String(waterFlowRate));
    }
     
	//readed the value for amount of water carried. Rotary angle is always set to a constant value, only changes on rotation
    milliLitresCarriedDec = analogRead(waterCapacity)*rotationRotaToMilliLetres; 

	//Rounds Down
    milliLitresCarried = floor((milliLitresCarriedDec)/10)*10;
	
	//Converts to DECimal for further calculation 
    milliLitresCarriedDec = milliLitresCarried;

	//Converts to DECimal for further calculation
    milliLitresConsumedDec = milliLitresConsumed;
	
	//Converts to DECimal for further calculation
    waterPercentDec = waterPercent;

	//Calculates percentage of water remaining for LED Display
    waterPercentDec = ((milliLitresCarriedDec - milliLitresConsumedDec) / milliLitresCarriedDec)*100 ; 

    waterPercent = round(waterPercentDec); //Convert Float to Decimal

	//Calls function to display the Level of water in the Hydration Pack
    displayLevels(digitalRead(BUTTONPIN));

	//do stuff that needs to be done every second
	oneSecTimer();

	//do stuff that needs to be done every 10 seconds
	tenSecTimer();

	//Calculate if a second has passed
    showTime = millis() - oldTime;
	//Execute every second
	if((showTime) > 1000) {

		// Removing the interupt listner so we can calcualte the value.
		detachInterrupt(waterSensorInterrupt);
	
		//Convert pulse count to litres by using time
		waterFlowRate = ((1000.0/(showTime)) * waterPulseCount) / calibrationFactor;

		//alter flow rate because it will continuously output a low decimal number and we don't want it tracking this.
		if(waterFlowRate < 0.14){
			waterFlowRate = 0;
		}
	
		//Convert from Flow rate to MilliLitres
		waterFlowMilliLitres = (waterFlowRate / 60) * 1000;
		//Add recent flow in last second to the total
		waterTotalMilliLitres += waterFlowMilliLitres;

		// check to see if water has been drunk and the flow has stopped, if so set the flag to measure water
		if(waterTotalMilliLitres > 0 && waterFlowRate == 0) {
			waterFlag = true;
		}
	
		// if we finished mesuring water, lets save the data
		if(waterFlag) {
			milliLitresConsumed += waterTotalMilliLitres;
			milliLitresSaved = waterTotalMilliLitres;
			publishGulps = true;
			waterTotalMilliLitres = 0;
			waterFlag = false;
		}

		//update EEPROM with Cunsumed water for persistance, so that if the photon is rebooted it will not loose the amount of water consumed
		EEPROM.update(addr,milliLitresConsumed);
		int value=0;
		EEPROM.get(addr, value);


		//reset water pulse count to capture next set of Pulses
		waterPulseCount = 0;

		//used for Debug, but want to avoid running all the time due to issues with the delay to the interupt
		//   Serial.print("Just Put Consumed to EEPROM value was: ");
		//   Serial.print(int(value));  // Print the integer part of the variable
		//    Serial.print(".");

		// Reataching the interupt listner to the pins
		attachInterrupt(waterSensorInterrupt, waterPulseCounter, FALLING);
    

		//Publish amount drunk in gulps - period if a drink has been taken
		if (publishGulps){
			publishGulps = false;
            Spark.publish("librato_Gulps", String(milliLitresSaved), 60, PRIVATE);
            milliLitresSaved = 0;
		}

		//reset Time for next second pass of main loop
		oldTime = millis();
	}

}


//Function to reset the counter and water consumed, can be triggered by rotary angle sensor being set to 0 and button being pressed, or API function being triggerd
int resetCounter(String resetvalue){
    Serial.print("  reset...............................................: ");             // Output separator
    Serial.print(resetvalue);
    Serial.println("reset......................................................");
    Spark.publish("reset",String(resetvalue));
    if (resetvalue=="reset"){
		milliLitresConsumed = 0;
		waterPercent = 100;
		EEPROM.put(addr,milliLitresConsumed);
		return 0;
      }
}

//Function to display Levels on 4 digit Display
void displayLevels(int num){
	//Check to see if button is NOT pressed
	if (num==0){
		//Change the display every 5 seconds between displaying the amount consumed and the percentage level
		if((millis() - oldTimeMaster) > 5000){
			//execute function to display water level
			waterLevel(waterPercent);
        }
        else {
			//execute function to display amount consumed
			dispWaterConsumed(milliLitresConsumed);  
		}
    }
	//if button is pressed display the amount of water stored in the hydration pack, when it was last filled
    else {
		//Function to display the amount of water carried
        dispWaterCapacity(milliLitresCarried);
    } 

}


//Function to accumulate the pulses from the Flow Sensor
void waterPulseCounter() {
  waterPulseCount++;
}

//Function to display the water consumed on the 4 digit Display
void dispWaterConsumed(unsigned int num){
	//built in function to decplay decimal numbers on 4 digit display
    display.showNumberDec(num);
}

//Function to display the water carried on the 4 digit Display
void dispWaterCapacity(unsigned int num){
	//built in function to decplay decimal numbers on 4 digit display
    display.showNumberDec(num);

}

//Function that is triggered when rotary angle sensor is turned, increments capacityChange to indicate change has occured 100 is arbitary
void waterCapacityChange(){
    capacityChange = capacityChange + 100 ;

}

//Function that displays the water level given a water percentage on the four digit display, SEG_# defined in variables to be vertical lines on display that equate to water level
void waterLevel(int waterPercentage){

    if(waterPercentage <= 100 && waterPercentage >= 81 ){
        display.setSegments(SEG_90);
    }

    if(waterPercentage <= 80 && waterPercentage >= 71 ){
        display.setSegments(SEG_80);
    }
	
    if(waterPercentage <= 70 && waterPercentage >= 61 ){
        display.setSegments(SEG_70);
    }
	
    if(waterPercentage <= 60 && waterPercentage >= 51){
        display.setSegments(SEG_60);
    }
	
    if(waterPercentage <= 50 && waterPercentage >= 41 ){
        display.setSegments(SEG_50);
    }
	
    if(waterPercentage <= 40 && waterPercentage >= 31 ){
        display.setSegments(SEG_40);
    }
	
    if(waterPercentage <= 30 && waterPercentage >= 21 ){
        display.setSegments(SEG_30);
    }
	
    if(waterPercentage <= 20 && waterPercentage >= 11 ){
        display.setSegments(SEG_20);
    }
	
	//When we  get to less than 10% show "LOW"
    if(waterPercentage <= 10 && waterPercentage > 0 ){
        display.setSegments(SEG_LO);
    }    
	
	//When we are out of water show "OUT"
    if(waterPercentage <= 0){
        display.setSegments(SEG_OUT);
    }
}

//Every second see if the capacity has changed
void oneSecTimer(){
	//see if a second has passed
	if((millis() - oldTime) >1000){
		//Detatch interupt for capacity to see if change has occured
		detachInterrupt(waterCapacityInterrupt);
		//Gather new Capacity
        capacityChangeINT = capacityChange;

        if ( capacityChange > 0){  
			//reset to 0
			capacityChange=0;

        }
    attachInterrupt(waterCapacityInterrupt, waterCapacityChange, CHANGE); 
 }
}


void tenSecTimer(){
	if((millis() - oldTimeMaster) > 10000) {
		
		Serial.println("still alive");
		//save the amount consumed to EEPROM so that we don't loose it if there is a reset of device or it looses power
        EEPROM.update(addr,milliLitresConsumed);
		
        int value=0;
		//Get value to make sure it is still acurtate
        EEPROM.get(addr, value);
        Serial.println(value);


		//send webhook to liberato to track historical data 
        Spark.publish("librato_TotalDrunk", String(milliLitresConsumed), 60, PRIVATE);

        Spark.publish("librato_Water_Carried", String(milliLitresCarried), 60, PRIVATE);

        Spark.publish("hydraFlow/MilliLitresConsumed", String(milliLitresConsumed));

        Spark.publish("hydraFlow/Percent Remaining", String(waterPercent));

		//Spark.publish("TotalMillileters", String(waterTotalMilliLitres));
        Spark.publish("hydraFlow/WaterCapacity", String(milliLitresCarried));

		//Spark.publish("flowrate", String(waterFlowRate));

		//reset 10 sec Timer
        oldTimeMaster = millis();
      }
}