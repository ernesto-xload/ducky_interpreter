/*
 *  Rubber Ducky script interpreter
 *
 *  Explanation: This code run Rubber Ducky scripts directly in cheap
 *               Atmega32u4 + microsd boards just copying to microSD.
 *               Also works in any Arduino Leonardo (original or clone) + SD board,
 *               please keep in mind connect CS pin to digital 4 or modify this code
 *
 *  Use:
 *    - Compile and load this code at board as Arduino Leonardo.
 *    - Rename your Rubber Ducky script to script.txt at microsd card
 *    - Insert microsd card at your board
 *    - Hack & fun! ;)
 *
 *  If you want to configure the kayboard layout download this library: https://github.com/ernesto-xload/arduino_keyboardlib
 *  http://sanchezpano.info
 *
 *  Version:		0.1 beta
 *  Design:		Ernesto Sanchez
 *  Implementation:	Ernesto Sanchez
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// CONFIG SECTION //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//#define DEBUG_ON //Configure serial debug _WARNING:_ If debug is enabled script will start when serial monitor is opened
#define BLINK_AT_END // Blink at end  or not
#define SCRIPT_FILENAME "script.txt"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include <SPI.h>
#include <SD.h>
#include <string.h>
#include "Keyboard.h"

//Global variables
File myFile;
int led = 13;
char command[255];
char readed_char;
int globalmodifiers=0;
unsigned long previous_line=0,current_line=0,next_line=0;
long repeat_times=-1,tmp_repeat_times=0;

//Functions
void print_ram(void);
int process_commad (char*);
int str_start(char*,char const*);
void process_string_commmand(void);

void setup()
{
  int special_command=0;
  unsigned int i=0,j=0;
  
  //Set led pin as output
  pinMode(led, OUTPUT); //LED
  digitalWrite(led, LOW); //Led turn-off

  #ifdef DEBUG_ON
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println(F("Rubber Ducky script interpreter for arduino"));
  Serial.println(F("By Ernesto a.k.a. Xload"));
  print_ram();
  #endif

  
  //Set SD card CS pin as output
  pinMode(4, OUTPUT);
  
  //Initializing SD card
  if (!SD.begin(4)) {
    #ifdef DEBUG_ON
    Serial.println(F("error beggining SD card"));
    #endif
  }

  delay(1000);
  
  // Read script file
  myFile = SD.open(SCRIPT_FILENAME);
  if (myFile) {
    while (myFile.available()) {
      readed_char=myFile.read();
      #ifdef DEBUG_ON
      Serial.print(" ");
      if((readed_char>32) && (readed_char<127)){ //Only print printable chars
        Serial.print(readed_char);
      }
      else {
        Serial.print(" ");
      }
      Serial.print(" ");
      Serial.print("0x");
      Serial.println(readed_char,HEX);
      #endif
      /*
      if(readed_char=='\x0a'){ // If readed_char is LF process it
        process_new_line();
      }
      */
      
      if(readed_char!='\x0d'){ // If readed_char is CR just ignore it (We only need LF for new line)
        command[i]=readed_char;
        command[i+1]='\x00'; // End string
        i++;
  
        // First process commands that need read entire line
        if(str_start(command,"STRING ")){
            //Special case STRING
            process_string_commmand();
            command[i-1]='\x00';
            command[0]='\x00';
            i=0;
        }
  
        if(str_start(command,"REM ")){
            //Special case REM
            #ifdef DEBUG_ON
            Serial.println(F("REM"));
            #endif
            //Read till LF
            while(!(readed_char=='\x0a')){
              readed_char=myFile.read();
            }
            process_new_line();
            command[0]='\x00';
            i=0;
        }
  
        if (str_start(command,"DELAY ")){
          #ifdef DEBUG_ON
          Serial.print(F("DELAY "));
          #endif
          while(!(readed_char=='\x0a')){
            readed_char=myFile.read();
            command[i]=readed_char;
            i++;
          }
          #ifdef DEBUG_ON
          Serial.println(atoi(command + 6));
          #endif
          delay(atoi(command + 6));
          process_new_line();
          command[i-1]='\x00';
          i=0;
          command[0]='\x00';
        }

        if (str_start(command,"REPEAT ")){
          #ifdef DEBUG_ON
          Serial.println(F("REPEAT"));
          #endif
          while(!(readed_char=='\x0a')){
            readed_char=myFile.read();
            command[i]=readed_char;
            i++;
          }
          
          tmp_repeat_times=atoi(command + 7);

          #ifdef DEBUG_ON
          Serial.print(F(" previous_line: "));
          Serial.println(previous_line);
          Serial.print(F(" tmp_repeat_times: "));
          Serial.println(tmp_repeat_times);
          Serial.print(F(" repeat_times:"));
          Serial.println(repeat_times);
          #endif

          if(repeat_times<0){ //is the first time that we process this REPEAT
            repeat_times=tmp_repeat_times; //set repeat_times to readed value
            current_line=previous_line;
            next_line=myFile.position();
            myFile.seek(previous_line); //Jump to file previous lune
          }

          if(repeat_times>0){ //a REPEAT is being processed
            repeat_times--; //decrease repeat_times
            current_line=previous_line;
            myFile.seek(previous_line); //Jump to file previous lune
          }

          if(repeat_times==0){ //REPEAT ended
            repeat_times=-1; //set repeat_times to -1
            myFile.seek(next_line); //Jump to next line
            next_line=0;
          }
          i=0;
          command[0]='\x00';
        }
  
        // Second process commands that are just keys and modifiers
        if (((readed_char==' ')||(readed_char=='+'))) {
          command[i-1]='\x00';
          if (strlen(command)>0){
            process_commad(command);
          }
          i=0;
        }
  
        if ((readed_char=='\x0a')) {
          command[i-1]='\x00';
          if (strlen(command)>0){
            process_commad(command);
            releaseallkeys();
            process_new_line();
          }
          ///process_new_line();
          i=0;
        }
      }
      else {
        #ifdef DEBUG_ON
        Serial.println(F("Readed 0x0D ==> Skip"));
        #endif
        ;
      }
    }
    myFile.close();
    command[i]='\x00';
    if (strlen(command)>0){
      process_commad(command);
    }
    releaseallkeys();
  }
  else {
    // Error opening script.txt
    // script.txt does not exist or there is a microSD problem
    #ifdef DEBUG_ON
    Serial.println(F("error opening script.txt"));
    #endif
    while(1);
 
  }
  
}

void loop()
{
  #ifdef BLINK_AT_END
  digitalWrite(led, HIGH);
  delay(1500);
  digitalWrite(led, LOW);
  delay(1500);
  #endif
}

/*
 * All commands: https://github.com/hak5darren/USB-Rubber-Ducky/wiki/Duckyscript
 * TODO:
 * - NUMLOCK
 * - SCROLLLOCK
 * - DEFAULT_DELAY DEFAULTDELAY
 */

int process_commad (char* command) {
  int i=0;
  #ifdef DEBUG_ON
  Serial.println(F("Process_commad()"));
  Serial.print(F(" Command: \""));
  Serial.print(command);
  Serial.println("\"");
  #endif

  // Modifer keys
  
  // GUI or WINDOWS
  if (!strcmp(command,"WINDOWS")||!strcmp(command,"GUI")) {
    globalmodifiers=KEY_LEFT_GUI;
    sendkey(0,KEY_LEFT_GUI);
    return 0;
  }

  // ALT
  if (!strcmp(command,"ALT")) {
     sendkey(0,KEY_LEFT_ALT);
     return 0;
  }

  // CTRL
  if (!strcmp(command,"CTRL")) {
     sendkey(0,KEY_LEFT_CTRL);
     return 0;
  }

  // SHIFT
  if (!strcmp(command,"SHIFT")) {
     sendkey(0,KEY_LEFT_SHIFT);
     return 0;
  }

  
  //Single keyboard keys
  
  //F1-F2 single stroke key
  if (str_start(command,"F")) {
    sendkey(KEY_F1+((atoi(command+1))-1),0);
    return 0;
  }
  
  if (!strcmp(command,"TAB")) {
     sendkey(KEY_TAB,0);
     return 0;
  }
  
  if (!strcmp(command,"LEFTARROW")) {
     sendkey(KEY_LEFT_ARROW,0);
     return 0;
  }
  
  if (!strcmp(command,"RIGHTARROW")) {
     sendkey(KEY_RIGHT_ARROW,0);
     return 0;
  }
  
  if (!strcmp(command,"UPARROW")) {
     sendkey(KEY_UP_ARROW,0);
     return 0;
  }
  
  if (!strcmp(command,"DOWNARROW")) {
     sendkey(KEY_DOWN_ARROW,0);
     return 0;
  }

  if (!strcmp(command,"PRINTSCREEN")) {
     sendkey(KEY_PRINTSCREEN,0);
     return 0;
  }

  if (!strcmp(command,"MENU")||!strcmp(command,"APP")) {
    sendkey(KEY_MENU,0);
    return 0;
  }

  if (!strcmp(command,"ENTER")) {
    sendkey(KEY_RETURN,0);
    return 0;
  }
  
  if (!strcmp(command,"led_on")) { 
    digitalWrite(led, HIGH);
    return 0;
  }
  
  if (!strcmp(command,"led_off")) {
    digitalWrite(led, LOW);
    return 0;
  }

  if (!strcmp(command,"ESC")||!strcmp(command,"ESCAPE")) {
    sendkey(KEY_ESC,0);
    return 0;
  }

  if (!strcmp(command,"HOME")) {
    sendkey(KEY_HOME,0);
    return 0;
  }

  if (!strcmp(command,"SPACE")) {
    sendkey(' ',0);
    return 0;
  }

  if (!strcmp(command,"INSERT")) {
    sendkey(KEY_INSERT,0);
    return 0;
  }

  if (!strcmp(command,"DELETE")) {
    sendkey(KEY_DELETE,0);
    return 0;
  }
  
  if (!strcmp(command,"PAGEUP")) {
    sendkey(KEY_PAGE_UP,0);
    return 0;
  }

  if (!strcmp(command,"PAGEDOWN")) {
    sendkey(KEY_PAGE_DOWN,0);
    return 0;
  }

  if (!strcmp(command,"BREAK")||!strcmp(command,"PAUSE")) {
    delay(atoi(command + 6));
    return 0;
  }

  if (!strcmp(command,"END")) {
    sendkey(KEY_END,0);
    return 0;
  }

  for(i=0;i<strlen(command);i++){
    sendkey(command[i],0);
  }
/*
  if (strlen(command)==1) {
    sendkey(command[0],0);
  }
*/
  
  return 1;
}

void process_string_commmand(void){
  #ifdef DEBUG_ON
  Serial.print(F("STRING "));
  #endif
  readed_char=myFile.read();
  while(!(readed_char=='\x0a')){
    #ifdef DEBUG_ON
    Serial.print(readed_char);
    #endif
    Keyboard.print(readed_char);
    readed_char=myFile.read();
  }
  #ifdef DEBUG_ON
  Serial.println();
  #endif
  process_new_line();
}

void sendkey(int key, int modifiers){
  #ifdef DEBUG_ON
  Serial.println(F("SendKey:"));
  Serial.print(F(" key: "));
  Serial.println(key,HEX);
  Serial.print(F(" modifiers: "));
  Serial.println(modifiers,BIN);
  #endif
  if(modifiers!=0){
    Keyboard.press(modifiers);
  }
  if(key!=0){
    Keyboard.press(key);
  }
  }

void releaseallkeys(void){
  #ifdef DEBUG_ON
  Serial.println(F("Release all keys"));
  #endif
  globalmodifiers=0;
  Keyboard.releaseAll();
}

//return 1 if strlong start by strshort and 0 if not
int str_start(char*strlong,char const*strshort){
  int i,j=1;
  for(i=0;i<strlen(strshort);i++){
    if(strlong[i]!=strshort[i]) j=0;
  }
  return j;
}

void process_new_line(void){
  previous_line=current_line;
  current_line=myFile.position(); 
  #ifdef DEBUG_ON
  Serial.println(F("Process new line"));
  Serial.print(F(" previous_line: "));
  Serial.println(previous_line);
  Serial.print(F(" current_line: "));
  Serial.println(current_line);
  #endif
}

// Debug functions

#ifdef DEBUG_ON
void print_ram(void) {
  Serial.print(availableMemory());
  Serial.println(F(" bytes free of 2560 (RAM)"));
}
// this function return the number of bytes currently free in RAM
int availableMemory() {
  int size = 2560;
  
  byte *buf;

  while ((buf = (byte *) malloc(--size)) == NULL);

  free(buf);

  return size;
}
#endif


