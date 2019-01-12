#include "fakesd.h"
#include "string.h"
Fakesd::Fakesd(){
  //Aqui se puede hardcodear la string
  strcpy(scriptraw,"DELAY 3000\nGUI\nDELAY 1500\nSTRING terminal\nDELAY 1500\nENTER\nDELAY 1000\nSTRING echo Hello World!!!\nENTER");
  read_position = 0;
}
char Fakesd::read() {
  char ret = scriptraw[read_position];
  read_position++;
  return ret;
}

bool Fakesd::available(){
  char current = scriptraw[read_position];
  if(current == '\0'){
    return false;
  }
  return true;
}

unsigned long Fakesd::position(){
  return read_position;
}

bool Fakesd::seek(unsigned long new_position){
  read_position = new_position;
  return true;
}

void Fakesd::close(){
}
