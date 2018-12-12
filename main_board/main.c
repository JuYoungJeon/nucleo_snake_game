#include "mbed.h"
#include <Adafruit_ST7735.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BOARD_WIDTH 128
#define BOARD_HEIGHT 128
#define BLOCK_SIZE 4

typedef enum blocktype{BLANK, BOUNDARY, SNAKE, FOOD} blocktype;
typedef enum direction{NONE, UP, DOWN, RIGHT, LEFT} direction;
typedef enum status{GAMEON, GAMEOVER}status;
typedef struct snake_blocks{
    int x, y;
    struct snake_blocks* prev;
}snake_blocks;

Serial pc1(SERIAL_TX, SERIAL_RX, 115200);
Adafruit_ST7735 tft(D11,NC, D13,D10, D5, D4);
Ticker food_checker;
Serial bluetooth(D8, D2, 9600);
PwmOut buzzer(D3);

int gameStartBGMNum=14, gameOverBGMNum=6, moveBGMNum=2;
float freqGameStart[]={698.46,880.00,783.99,698.46,1396.91,698.46,698.46,880.00,932.33,1046.50,1174.66,1046.50,698.46,698.46};
float freqGameOver[]={261.63,246.94,233.08,220.00,207.65,196.00};
float freqMove[]={261.63, 196.00};
int beatGameStart[]={6,4,4,6,8,12,6,4,4,4,4,4,3,3};
int beatGameOver[]={8,4,8,4,16,4};
int beatMove[]={2, 4};

blocktype board[BOARD_HEIGHT/BLOCK_SIZE][BOARD_WIDTH/BLOCK_SIZE];
direction curDir = NONE;
status gameStatus=GAMEON;
int snakeSize=3;
float speed=0.5;
snake_blocks* snakeHead, *snakeTail;
int isFood=0, eaten_stat=0;
snake_blocks *firstNewBlock=NULL, *lastNewBlock=NULL;
int score=0;

void initConsole();
void gameStartBGM();
void gameOverBGM();
void moveBGM();
void displayConsole();
void initSnake();
void removeSnakeBlock();
void addSnakeBlock();
void change_dir();
void createFood();
status moveSnake();
status checkGameOver();

int main(void){
    buzzer=1.0;
    srand(time(NULL));

    while(!bluetooth.writeable());
    pc1.printf("Bluetooth starting...\r\n");
    bluetooth.printf("AT+RENEW\r\n");
    wait(1);
    bluetooth.printf("AT+ROLE0\r\n");
    while(bluetooth.readable())
        pc1.putc(bluetooth.getc());
    while(bluetooth.getc()!='&'){}
    bluetooth.putc('&');

    tft.initR();
    tft.fillScreen(0x0000);

    initSnake();
    initConsole();
    displayConsole();
    gameStartBGM();
    food_checker.attach(&createFood, 0.001);

    pc1.printf("start\r\n");
    bluetooth.attach(&change_dir);      

    //pc1.attach(callback(change_dir));

    while(1){
      if(curDir==NONE) continue;
      gameStatus=moveSnake();
      if(gameStatus==GAMEOVER)
          break;
      displayConsole();
      wait(speed);
    }
    gameOverBGM();
    pc1.printf("end\r\n");
}

void gameStartBGM(){
    int period_us;
    int beat_ms;

    for(int i=0;i<gameStartBGMNum; i++){
        buzzer=0.9;
        period_us=1000000/freqGameStart[i];
        beat_ms=62.5*beatGameStart[i];
        buzzer.period_us(period_us);
        wait_ms(beat_ms);
    }
    buzzer=1.0;
}

void gameOverBGM(){
    int period_us;
    int beat_ms;

    for(int i=0;i<gameOverBGMNum; i++){
        buzzer=0.9;
        period_us=1000000/freqGameOver[i];
        beat_ms=62.5*beatGameOver[i];
        buzzer.period_us(period_us);
        wait_ms(beat_ms);
    }
    buzzer=1.0;
}

void moveBGM(){
    int period_us;
    int beat_ms;

    for(int i=0;i<moveBGMNum; i++){
        buzzer=0.9;
        period_us=1000000/freqMove[i];
        beat_ms=62.5*beatMove[i];
        buzzer.period_us(period_us);
        wait_ms(beat_ms);
    }
    buzzer=1.0;
}

void createFood(){
    while(!isFood){
        int foodX, foodY;
        foodX=rand()%(BOARD_WIDTH/BLOCK_SIZE-1);
        foodY=rand()%(BOARD_HEIGHT/BLOCK_SIZE-1);
        if(board[foodY][foodX]==BLANK){
          board[foodY][foodX]=FOOD;
          isFood=1;
        }else
          continue;
    }
}

void change_dir(){
  char ch; 
  if(bluetooth.getc()=='h'){
    ch = bluetooth.getc();
    if(ch=='4'){
        if(curDir!=RIGHT){
            curDir=LEFT;
            moveBGM();
        }
    }else if(ch=='3'){
        if(curDir!=LEFT){
            curDir=RIGHT;
            moveBGM();
        }
    }else if(ch=='1'){
        if(curDir!=DOWN){
            curDir=UP;
            moveBGM();
        }
    }else if(ch=='2'){
        if(curDir!=UP){
            curDir=DOWN;
            moveBGM();
        }
    }
  }
}

void initConsole(){
    for(int i=0; i<BOARD_HEIGHT/4; i++){
        for(int j=0; j<BOARD_WIDTH/4; j++){
            if(i==0||j==0||i==BOARD_HEIGHT/BLOCK_SIZE-1||j==BOARD_WIDTH/BLOCK_SIZE-1)
                board[i][j]=BOUNDARY;
            else if(i==BOARD_HEIGHT/8 && (j>(BOARD_WIDTH/8)-2&&j<(BOARD_WIDTH/8)+2))
                board[i][j]=SNAKE;
            else
                board[i][j]=BLANK;
        }
    }
}

void initSnake(){
    snakeHead = (snake_blocks *)malloc(sizeof(snake_blocks));
    snakeHead->x= BOARD_WIDTH/BLOCK_SIZE/2+1;
    snakeHead->y= BOARD_WIDTH/BLOCK_SIZE/2;
    snakeHead->prev= NULL;

    snake_blocks *temp = (snake_blocks *)malloc(sizeof(snake_blocks));
    temp->x= snakeHead->x-1;
    temp->y= snakeHead->y;
    temp->prev=snakeHead;

    snakeTail = (snake_blocks *)malloc(sizeof(snake_blocks));
    snakeTail->x= temp->x-1;
    snakeTail->y=temp->y;
    snakeTail->prev = temp;

}

void addSnakeBlock(){
    snake_blocks *temp = (snake_blocks *)malloc(sizeof(snake_blocks));
    if(curDir==RIGHT){
        temp->x=snakeHead->x+1;
        temp->y= snakeHead->y;
    }else if(curDir==LEFT){
        temp->x=snakeHead->x-1;
        temp->y=snakeHead->y;
    }else if(curDir==UP){
        temp->x=snakeHead->x;
        temp->y=snakeHead->y+1;
    }else if(curDir==DOWN){
        temp->x=snakeHead->x;
        temp->y=snakeHead->y-1;
    }
    temp->prev=NULL;
    snakeHead->prev=temp;
    snakeHead=temp;
}

void removeSnakeBlock(){
    snake_blocks *temp = snakeTail;
    snakeTail = snakeTail->prev;
    free(temp);
}

void displayConsole(){
    for(int i=0; i<BOARD_HEIGHT/4; i++){
        for(int j=0;j<BOARD_WIDTH/4; j++){
            if(board[i][j]==BOUNDARY){
                tft.fillRect(i*4, j*4, 3, 3, 0xFFFF);
            }else if(board[i][j]==SNAKE){
                tft.fillRect(i*4, j*4, 3, 3, 0x07E0);
            }else if(board[i][j]==FOOD){
                tft.fillRect(i*4, j*4, 3, 3, 0x001F);
            }else if(board[i][j]==BLANK){
                tft.fillRect(i*4, j*4, 3, 3, 0x0000);
            }
        }
    }
}

status checkGameOver(){
    if(board[snakeHead->y][snakeHead->x]==SNAKE||board[snakeHead->y][snakeHead->x]==BOUNDARY){
        return GAMEOVER;
    }
    return GAMEON;
}

status moveSnake(){
    status stat;
    addSnakeBlock();
    stat=checkGameOver();
    if(board[snakeHead->y][snakeHead->x]==FOOD){
        snake_blocks *temp = (snake_blocks*)malloc(sizeof(snake_blocks));
        temp->x= snakeHead->x;
        temp->y= snakeHead->y;
        temp->prev=NULL;
        if(firstNewBlock==NULL){
          firstNewBlock=temp;
          lastNewBlock=firstNewBlock;
        } else {
          lastNewBlock->prev=temp;
          lastNewBlock=temp;
        }
        isFood=0;
        eaten_stat++;
        score+=10;
        if(score%30==0){
            speed/=2;
            pc1.printf("%.3f\r\n", speed);
        }
        createFood();
    }
    board[snakeTail->y][snakeTail->x]=BLANK;
    board[snakeHead->y][snakeHead->x]=SNAKE;
    if(eaten_stat>0&&firstNewBlock->x==snakeTail->prev->x&&firstNewBlock->y==snakeTail->prev->y){
        snakeSize++;
        eaten_stat--;
        snake_blocks *temp= firstNewBlock;
        firstNewBlock=firstNewBlock->prev;
        free(temp);
    }else{
        removeSnakeBlock();
    }
    return stat;
}
