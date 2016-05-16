/*
 * ################################################################################################
 * HEADER INFORMATION
 *     > Includes
 *     > Preprocessor definitions
 *     > Macro functions
 *     > Function definitions
 *     > Global variables
 * INCLUDES
 * PREPROCESSOR DEFINITIONS
 *     > Pin Aliases
 *         Pin aliases serve to clear understanding of code by hiding raw numbers and binary behind
 *         a recognizable name associated with a functionality in the end product
 *     > Enumerations and Other Integer Representations
 *         This section serves to abstract counting away from the original code to make it easier
 *         for a casual viewer to see what the decisions being made by the program are.
 *     > Special Characters
 *         These are special ascii characters
 *     > Flags
 *         These codes are used for the representation of both errors and notifications on the
 *         status of a single run of the program.
 * MACRO FUNCTIONS
 *     > Pin Checking Functions
 *         These macro functions serve to hide binary comparison in registers on the system behind
 *         a simple text representation of what they are accomplishing
 *     > Angle Calculation Functions
 *         Functions used for abstracting long or multiline calculations in order to preserve code
 *         readability.
 * FUNCTION DEFINITIONS
 * GLOBAL VARIABLES
 *     > Timer Counts
 *         The timer counts variables serve to support the run of the code by keeping the start and
 *         stop of the timer in memory for comparison
 *     > State Variables
 *         The state variables keep track of the position in the the run that the program should be
 *         running through
 *     > Phase Measurement Variables
 *         These are used to store the actual information on measurement of the phase offset of the
 *         incoming signal
 *
 * NOTES
 *     > I'm working to remove as many global variables as possible as using them is poor coding
 *       practice
 *     > I resisted the urge to use the register keyword as it's basically a placebo in this and
 *       other simple programs
 * ################################################################################################
 */
 
/* ########################################### Includes ########################################### */
#include "eRCaGuy_Timer2_Counter.h"  // Precision Library
/* ################################################################################################ */
 
/* ################################### Preprocessor Definitions ################################### */
//pin aliases
#define PPS         2         //pulse pin set to digital pin 2
#define Hz60        3         //input signal pin set to digital pin 3
#define PPSA        B00000100 //alias for the pps pin in the PIND register on the AtMEGA 328p
#define Hz60A       B00001000 //alias for the signal pin in the PIND register on the AtMEGA 328p
#define REG_EMPTY   B00000000 //empty register alias, a.k.a. 0

//enums etc
#define LEAD        1         //the signal has a leading offset
#define LAG         2         //the signal has a lagging offset
#define NO_LOOP     0         //no loop indicator
#define LOOP        1         //loop indicator
#define INITIAL     0         //initial state
#define CALIBRATE   1         //calibration state
#define READ        2         //reading state
#define SET_SCALE   4         //state for setting scaling factors
#define BAUD        9600      //baud rate

//special characters
#define NUL         '\000'    //null character
#define EOT         '\004'    //end of transmission character
#define ACK         'a'//'\006'    //acknowledgment character
#define LF          '\012'    //line feed character
#define DC2         'c'//'\022'    //control character 2
#define DC3         'b'//'\023'    //control character 3

//flags
#define EMPTY_FLAGS B00000000 //empty flags, equivalent to 0
#define CLBRTN_LEAD B00000001 //calibration indicated lead in signal phase
#define CLBRTN_LAG  B00000010 //calibration indicated lag in signal phase
#define READ_LEAD   B00000100 //calibration indicated lead in signal phase
#define READ_LAG    B00001000 //calibration indicated lag in signal phase
#define STATE_ERROR B00010000 //error in the state machine occurred
#define CLBRTN_ERR  B00100000 //error in calibration occurred
#define READ_ERROR  B01000000 //error in signal reading occurred
#define COMPLETE    B10000000 //complete run
/* ################################################################################################ */

/* ####################################### Macro Functions ######################################## */
//pin checking functions
#define PIND_CALC(x,y) ( ( PIND & x ) == y )         //conditional for calculating bit state
#define PPS_LOW        PIND_CALC( PPSA, REG_EMPTY )  //conditional to check if pulse is low
#define PPS_HIGH       PIND_CALC( PPSA, PPSA )       //conditional to check if pulse is high
#define Hz60_LOW       PIND_CALC( Hz60A, REG_EMPTY ) //conditional to check if signal is low
#define Hz60_HIGH      PIND_CALC( Hz60A, Hz60A )     //conditional to check if signal is high

//angle calculation functions
#define GET_ANGLE(x,y) ( 180 * ( 1.0 - ( x / y ) ) ) //generates an angle from a value and reference
/* ################################################################################################ */

/* ##################################### Function Definitions ##################################### */
void           calibrate( );          //calibrates the device for readings
void          get_offset( );          //returns the phase offset in degrees
float generate_reference( int, int ); //returns the calculated reference
float     measure_offset( int );      //returns the calculated offset
/* ################################################################################################ */

/* ####################################### Global Variables ####################################### */
//timer counts
float pulseStart;              //the beginning of the counting
float pulseStop;               //the end of the counting

//state variables
int   state     = INITIAL;      //the stage in the process
int   phase     = 0;           //lead or lag
int   repeat    = NO_LOOP;     //

//phase measurement variables
float reference = 0.0;         //measured single cycle duration for calculating phase offset
float angle     = 0.0;         //Angle in degrees
float prescale  = 42.53;       // prescale adjustment based on PWM of trimble signal

//other
int   flags     = EMPTY_FLAGS; //indicators as to program run performance
char  input     = '\000';      //

/* ################################################################################################ */




/*
 * ################################################################################################
 * MAIN PROGRAM INFORMATION
 *     > Setup
 *     > Loop
 *     > Others
 * ************************************************************************************************
 * ******************************************** NOTICE ********************************************
 * THIS PROGRAM WAS RE-WRITTEN BY PHILIP WASHY FOR THE SPRING 2016 SENIOR DESIGN PROJECT MADE FOR
 * DR. GEORGE KUSIC. ALL CODE IS AVAILABLE FOR USE BY MAKING A REQUEST TO DR. KUSIC. ORIGINAL
 * CREDIT FOR THE CODE GOES TO H. KHALID, A. STIMMELL, R. LOIS, C. KENNEDY, AND DR. KUSIC AT THE
 * ELECTRICAL ENGINEERING DEPARTMENT AT THE SWANSON SCHOOL OF ENGINEERING'S COMPUTER AND ELECTRICAL
 * ENGINEERING DEPARTMENT, UNIVERSITY OF PITTSBURGH.
 * ************************************************************************************************
 * ************************************************************************************************
 * ################################################################################################
 */


/* ############################################ Setup ############################################# */
//setup code
void setup() 
{
  //setup the eRCAGuy PWM timer
  timer2.setup();
  
  //set the baud rate on the serial line
  Serial.begin( BAUD );
  
  //set the pin modes for pins in use
  pinMode( PPS,  INPUT );  //GPS pulse pin set to an input
  pinMode( Hz60, INPUT ); //Input square wave 
}
/* ################################################################################################ */

/* ############################################# Loop ############################################# */
//main loop
void loop() 
{
  switch( state ) {
    case INITIAL: {
      while( Serial.available() == 0 );
        input = Serial.read();
        if( input == ACK ) {
          state++;
          timer2.reset();
          repeat = NO_LOOP;
        } else if( input == DC3 ) {
          state++;
          timer2.reset();
          repeat = LOOP;
        } else if( input == DC2 ) {
          state = SET_SCALE;
        }
    } break;
    
    case SET_SCALE: {
      while( Serial.available() == 0 );
      prescale = Serial.parseFloat();
      state = INITIAL;
    } break;
    
    default: {
      pulseStart = timer2.get_count();
      delay(500);
      calibrate();
      
      state++;
      get_offset();
  
      flags |= (state>=2)? COMPLETE : EMPTY_FLAGS;
      
      Serial.print( flags, BIN );
      Serial.print( LF );
      
      input = ( Serial.available() )? Serial.read() : input;
      repeat = ( input == ACK )? NO_LOOP : LOOP;
      
      if( repeat == NO_LOOP ) {
        //Serial.print( EOT );
        state = INITIAL;
      } else {
        timer2.reset();
        state = CALIBRATE;
      }
      
      flags = EMPTY_FLAGS;
    } break;
  }
  
}
/* ################################################################################################ */

/* ############################################ Others ############################################ */
/*        ################################### Calibrate ####################################        */
void calibrate( ) {
  while( PPS_LOW );                    //wait for a pulse

  //if( PPS_HIGH ) {                    //ensure pulse actually went high
    phase = ( Hz60_LOW )? LAG : LEAD; //set the lead/lag

    switch( phase ) {
      
      case LEAD: {                    // Hz60 leads PPS 
        reference = generate_reference( Hz60A, REG_EMPTY );
        Serial.print( reference );   
        Serial.print( ',' );
        flags |= CLBRTN_LEAD;
      } break; // 60 Hz "on"    

      case LAG: {                   // Hz60 lags the PPS
        reference = generate_reference( REG_EMPTY, Hz60A );
        Serial.print( reference );
        Serial.print( ',' );
        flags |= CLBRTN_LAG;
      } break; // 60 Hz "off" 
    
      default: {
        flags |= CLBRTN_ERR;
      } break; //default
    
    } //END OF SWITCH
    flags |= ( state != CALIBRATE )? STATE_ERROR : EMPTY_FLAGS; //check for state error;
  //} //PPS_HIGH 
}
/*        ##################################################################################        */
/*        ################################## Get Offset ####################################        */
void get_offset( ) {
  float startRead, time;
  //Serial.println( "Getting offset" );
  while( PPS_LOW );             //wait for next PPS
  
 // if( PPS_HIGH ) {             //ensure the pulse is actually high
    phase = ( Hz60_LOW )? LAG : LEAD;
    startRead = timer2.get_count();
    
    switch(phase) {
      
      case LEAD: {             // Hz60 leads PPS 
        time = measure_offset( startRead, Hz60A );
        angle = GET_ANGLE( time, reference ) - prescale;
        Serial.print( time );
        Serial.print( ',' );
        Serial.print( angle );
        Serial.print( ',' );
        flags |= READ_LEAD;
      } break;   // 60 Hz "on"    

      case LAG: {              // Hz60 lags the PPS
        time = measure_offset( startRead, REG_EMPTY );
        
        angle = GET_ANGLE( time, reference ) - ( 180.00 + prescale );
        Serial.print( time );
        Serial.print( ',' );
        Serial.print( angle );
        Serial.print( ',' );
        flags |= READ_LAG;
      } break;  //60 Hz "off"  
      
      default: {
        flags |= READ_ERROR;
      } break;
    } //END OF SWITCH
    
    flags |= ( state != READ )? STATE_ERROR : EMPTY_FLAGS; //check for state error
 // }//PPS_HIGH
}
/*        ##################################################################################        */
/*        ############################### Generate Reference ###############################        */
float generate_reference( int startEdge, int stopEdge ) {
  float startRead, stopRead;
  
  while( PIND_CALC( Hz60A, startEdge ) ); //spin while 60 Hz is not on the start edge
          
  startRead = timer2.get_count();
          
  while( PIND_CALC( Hz60A, stopEdge ) ); //spin while 60 Hz is low
  
  stopRead = timer2.get_count();
  
  return ( stopRead - startRead );
}
/*        ##################################################################################        */
/*        ################################# Measure Offset #################################        */
float measure_offset( float startRead, int stopEdge ) {
  float stopRead;
          
  while( PIND_CALC( Hz60A, stopEdge ) ); //spin while 60 Hz is low
  
  stopRead = timer2.get_count();
  
  return ( stopRead - startRead );
}
/*        ##################################################################################        */
/* ################################################################################################ */
