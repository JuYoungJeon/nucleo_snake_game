#include "mbed.h"
#include <Adafruit_ST7735.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BOARD_WIDTH 128
#define BOARD_HEIGHT 128
#define BLOCK_SIZE 4

typedef enum blocktype{BLANK, BOUNDARY, SNAKE, FOOD} blocktype;
typedef enum direction{UP, DOWN, RIGHT, LEFT} direction;
typedef enum status{GAMEON, GAMEOVER}status;

typedef struct snake_blocks{
    int x, y;
    struct snake_blocks* prev;
}snake_blocks;

Serial pc1(SERIAL_TX, SERIAL_RX, 115200);
Adafruit_ST7735 tft(D11,NC, D13,D10, D5, D4);
Ticker food_checker;

blocktype board[BOARD_HEIGHT/BLOCK_SIZE][BOARD_WIDTH/BLOCK_SIZE];
direction curDir = RIGHT; 
status gameStatus=GAMEON;
int snakeSize=3;
float speed=0.5;
snake_blocks* snakeHead, *snakeTail;
int isFood=0, eaten_stat=0; 
int newBlockX=BOARD_WIDTH/BLOCK_SIZE, newBlockY=BOARD_HEIGHT/BLOCK_SIZE;
int score=0;

void initConsole();
void displayConsole();
void initSnake();
void removeSnakeBlock();
void addSnakeBlock();
void change_dir();
void createFood();
status moveSnake();
status checkGameOver();

int main(void){
    srand(time(NULL));
    
    tft.initR();
    tft.fillScreen(0x0000);
    
    initSnake();
    initConsole();
    displayConsole();
    food_checker.attach(&createFood, 0.001);
    
    pc1.printf("start\r\n");
    pc1.attach(callback(change_dir));
    
    while(1){
        gameStatus=moveSnake();
        if(gameStatus==GAMEOVER)
            break;
        displayConsole();
        wait(speed);
    }
    pc1.printf("end\r\n");
}

void createFood(){
    if(!isFood){
        int foodX, foodY;
        foodX=rand()%(BOARD_WIDTH/BLOCK_SIZE-1);
        foodY=rand()%(BOARD_HEIGHT/BLOCK_SIZE-1);
        if(foodX==0) foodX++;
        if(foodY==0) foodY++;
        board[foodY][foodX]=FOOD;
        isFood=1;
    }
}

void change_dir(){
    char ch;
    ch=pc1.getc();
    pc1.putc(ch);
    
    if(ch=='a'){
        if(curDir!=RIGHT)
            curDir=LEFT;
    }else if(ch=='d'){
        if(curDir!=LEFT)
            curDir=RIGHT;
    }else if(ch=='w'){
        if(curDir!=DOWN)
            curDir=UP;
    }else if(ch=='s'){
        if(curDir!=UP)
            curDir=DOWN;
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
        newBlockX=snakeHead->x;
        newBlockY=snakeHead->y;
        isFood=0;
        eaten_stat=1;
        score+=10;
        if(score%30==0){
            speed/=2;
            pc1.printf("%.3f\r\n", speed);
        }
        createFood();
    }
    board[snakeTail->y][snakeTail->x]=BLANK;
    board[snakeHead->y][snakeHead->x]=SNAKE;
    if(eaten_stat==1&&newBlockX==snakeTail->prev->x&&newBlockY==snakeTail->prev->y){
        snakeSize++;
        eaten_stat=0;
    }else{
        removeSnakeBlock(); 
    }
    return stat;
}
