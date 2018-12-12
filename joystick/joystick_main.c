#include "mbed.h"

typedef enum direction{NONE, UP, DOWN, RIGHT, LEFT} direction;

Serial pc(SERIAL_TX, SERIAL_RX, 115200);
Serial bluetooth(D8, D2, 9600);
AnalogIn A_x(A0);
AnalogIn A_y(A1);

direction curDir=NONE;
direction prevDir=NONE;

void readDir();

int main(void){
  while(!bluetooth.writeable());
  pc.printf("Bluetooth starting...\r\n");

  bluetooth.printf("AT+RENEW"); wait(0.5);
  bluetooth.printf("AT+IMME1"); wait(0.5);
  bluetooth.printf("AT+ROLE1"); wait(0.5);
  while(bluetooth.readable()) pc.putc(bluetooth.getc());
  wait(2);
  bluetooth.printf("AT+CON0CB2B778AFB7");
  pc.printf("Connecting...\r\n");
  wait(2);

  bluetooth.putc('&');
  pc.printf("Waiting for ACK\r\n");
  while(bluetooth.getc()!='&'){bluetooth.putc('&');}
  pc.printf("ACK received\r\n");

  while(1){
      readDir();
      if(prevDir!=curDir&&bluetooth.writeable()){
        prevDir=curDir;
        bluetooth.printf("h%d", curDir);
        pc.printf("current direction: %d\r\n", curDir);
      }
  }
}

void readDir(){
  float Ax, Ay;
  Ax = A_x.read();
  Ay = A_y.read();

  //up
  if((0.24f < Ax && Ax < 0.76f) && (0.74f < Ay && Ay < 1.01f)){
      if(curDir!=DOWN)
          curDir=UP;
  }
  //down
  else if((0.24f < Ax && Ax < 0.76f) && (-0.01f < Ay && Ay < 0.26f)){
      if(curDir!=UP)
          curDir=DOWN;
  }
  //left
  else if((-0.01f < Ax && Ax < 0.26f) && (0.24f < Ay && Ay < 0.76f)){
      if(curDir!=RIGHT)
          curDir=LEFT;
  }
  //right
  else if((0.74f < Ax && Ax < 1.01f) && (0.24f < Ay && Ay < 0.76f)){
      if(curDir!=LEFT)
          curDir=RIGHT;
  }
}
