/*
    +------------------------------------------------------+
    |       FINAL PROJECT OF IOT AND EMBEDDED SYSTEM       |
    | Project name: Battle Snake Game control by 2 MPU6050 |
    |GROUP: HẢO HÁN                                        |
    |1. TRẦN NGỌC ĐOÀN      MSSV: 19146175                 |
    |2. TRƯƠNG DUY KHA      MSSV: 19146015                 |
    |3. CHÂU LÊ TUẤN VỸ     MSSV: 19146047                 |
    +------------------------------------------------------+
*/
#include <stdio.h>
#include <time.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <wiringPiSPI.h>
#include <stdlib.h>
#include <unistd.h>
#include <mysql.h>

#define SMPLRT_DIV      25 //0x19
#define CONFIG          26 //0x1A
#define GYRO_CONFIG     27 //0x1B
#define ACCEL_CONFIG    28 //0x1C
#define INT_ENABLE      56 //0x38
#define PWR_MGMT_1     107 //0x6B
#define Acc_X           59
#define Acc_Y           61
#define Acc_Z           63
#define Temp            65 //0x41
#define channel         0
#define Start           37
#define Stop            35

int mpu_1, mpu_2; 
int score_snake_1; 
int score_snake_2;
int winner1, winner2;
int dir_1 = 2;
int dir_2 = 2;
int flag;

uint8_t hex2dec(uint8_t d) 
{
	uint8_t h;
    h = (d>>4)*10 + (d&0x0F);

    return h;
}
uint8_t dec2hex(uint8_t d)
{
    uint8_t h;
    h = (d/10 << 4) | (d%10);

    return h;
}
void send_data(uint8_t address, uint8_t value){
    uint8_t data[2];
    data[0] = address;
    data[1] = value;
    wiringPiSPIDataRW(channel,data,2);
}

void Init_max7219(void)
{
    //Decode mode: 0xFF
    send_data(0x09,0xFF); 
    //intensity : 0x0A0B
    send_data(0x0A,0x03);
    //scan limit
    send_data(0x0B,0x07);
    //no shutdown, display test off
    send_data(0x0C,1);
    send_data(0x0F,0);
}
int16_t read_sensor(int mpu, unsigned char sensor)
{
    int16_t high, low, data;
    high = wiringPiI2CReadReg8(mpu, sensor); //---> 0xF6
    low = wiringPiI2CReadReg8(mpu, sensor + 1); //---> 0x13
    data = (high<<8) | low;

    return data;
}
void Init_6050(int mpu)
{
    //register 25->28, 56, 107
    wiringPiI2CWriteReg8(mpu, SMPLRT_DIV, 15);     //Sample rate 500Hz
    wiringPiI2CWriteReg8(mpu, CONFIG, 0);    //Khong su dung nguon xung ngoai, On DLFP //98Hz
    wiringPiI2CWriteReg8(mpu, GYRO_CONFIG, 0x08);    //gyro FS: +- 500o/s
    wiringPiI2CWriteReg8(mpu, ACCEL_CONFIG, 0x10);    //acc FS: +- 8g
    wiringPiI2CWriteReg8(mpu, INT_ENABLE, 1);    //mo interrupt cua data ready
    wiringPiI2CWriteReg8(mpu, PWR_MGMT_1, 0x01);    //chon nguon xung Gyro X
}
void show(uint8_t value[8]) //Function Show direction
{
    for (uint8_t i = 0; i < 8; i++)
    {
        send_data(i+1, value[i]);
    }
}
void start()
{
    if(digitalRead(Start)==1)
    {
        delay(10);     
        if(digitalRead(Start)==1)
        {
            flag = 1;
        }
    }
}
void stop()
{
    if(digitalRead(Stop)==1)
    {
        delay(10);     
        if(digitalRead(Stop)==1)
        {
            flag = 0;
        }
    }
}
void UP() //show max7219 UP
{
    send_data(5, 0x01);
    send_data(6, 0x67);
    send_data(7, 0x3E);
    send_data(8, 0x01);
}
void DO() //show max7219 DO
{
    send_data(5, 0x01);
    send_data(6, 0x7E);
    send_data(7, 0x3D);
    send_data(8, 0x01);
}
void LE() //show max7219 LE
{
    send_data(5, 0x01);
    send_data(6, 0x4F);
    send_data(7, 0x0E);
    send_data(8, 0x01);
}
void RI() //show max7219 RI
{
    send_data(5, 0x01);
    send_data(6, 0x30);
    send_data(7, 0x77);
    send_data(8, 0x01);
}
void changedirection(float pitch, float roll, int dir) //show max7219 direction of 2 mpu, but just use it to check direction
{
        if (pitch < -10)
        {
            dir = 1; //go up
            UP();
        }
        if (pitch > 10)
        {
            dir = 3; //go down
            DO();
        }
        if (roll > 10)
        {
            dir = 2; //go right
            RI();
        }
        if (roll < -10)
        {
            dir = 4; //go left
            LE();
        }
}
void showscore_1(int value) //show score 1
{
    int a = value%10;
    int b = value/10;
    send_data(8, 0x0A);
    send_data(7, 0x0A);
    send_data(6, b);
    send_data(5, a);
}
void showscore_2(int value) //show score 2
{
    int a = value%10;
    int b = value/10;
    send_data(4, 0x0A);
    send_data(3, 0x0A);
    send_data(2, b);
    send_data(1, a);
}
int main(void)
{
    //Mysql
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    char *server = "192.168.39.226";
    char *user = "rasp";
    char *password = "password";
    char *database = "snakegame";

    //setup SPI interface
    wiringPiSPISetup(channel,8000000);
    //set operational mode max7219
    Init_max7219();
    //Setup giao tiep I2C
    mpu_1 = wiringPiI2CSetup(0x68); //Address of mpu 1
    mpu_2 = wiringPiI2CSetup(0x69); //Address of mpu 2
    //Thiet lap che do cho MPU6050
    Init_6050(mpu_1);
    Init_6050(mpu_2);
    //Khai bao ngat
    wiringPiSetupPhys();
    pinMode(Start, INPUT);
    pinMode(Stop, INPUT);
    wiringPiISR(Start,INT_EDGE_RISING,&start);
    wiringPiISR(Stop,INT_EDGE_RISING,&stop);
    uint8_t STOP[8] = {0x01, 0x01, 0x67, 0x7E, 0x0F, 0x5B, 0x01, 0x01};
    uint8_t PLAY[8] = {0x01, 0x01, 0x33, 0x77, 0x0E, 0x67, 0x01, 0x01};
    uint8_t P1[8] = {0x0A, 0x0A, 0x0A, 0x01, 0x0E, 0x0A, 0x0A, 0x0A};
    uint8_t P2[8] = {0x0A, 0x0A, 0x0A, 0x02, 0x0E, 0x0A, 0x0A, 0x0A};
    uint8_t TIE[8] = {0x0A, 0x02, 0x0E, 0x0A, 0x0A, 0x01, 0x0E, 0x0A};
    int direction_1, direction_2, score_1, score_2, start_1, start_2, stop_1, stop_2, winner_1, winner_2;
    char snake_1[5] = {direction_1, score_1, start_1, stop_1, winner_1};
    char snake_2[5] = {direction_2, score_2, start_2, stop_2, winner_2};
    while(1)
    {
        //Read data from mpu 1
        float Ax_1 = (float)read_sensor(mpu_1, Acc_X)/4096.0;
        float Ay_1 = (float)read_sensor(mpu_1, Acc_Y)/4096.0;
        float Az_1 = (float)read_sensor(mpu_1, Acc_Z)/4096.0;
        float pitch_1 = atan2(Ax_1, sqrt(pow(Ay_1, 2) + pow(Az_1, 2)))*180/M_PI;
        float roll_1 = atan2(Ay_1, sqrt(pow(Ax_1, 2) + pow(Az_1, 2)))*180/M_PI;
        printf("Pitch_1 %.3f, Roll_1 %.3f\n", pitch_1, roll_1);
        //Read data from mpu 2
        float Ax_2 = (float)read_sensor(mpu_2, Acc_X)/4096.0;
        float Ay_2 = (float)read_sensor(mpu_2, Acc_Y)/4096.0;
        float Az_2 = (float)read_sensor(mpu_2, Acc_Z)/4096.0;
        float pitch_2 = atan2(Ax_2, sqrt(pow(Ay_2, 2) + pow(Az_2, 2)))*180/M_PI;
        float roll_2 = atan2(Ay_2, sqrt(pow(Ax_2, 2) + pow(Az_2, 2)))*180/M_PI;
        printf("Pitch_2 %.3f, Roll_2 %.3f\n", pitch_2, roll_2);
        delay(100);
        //Show Max7219 direction to check
        //changedirection(pitch_1, roll_1, dir_1);
        //changedirection(pitch_2, roll_2, dir_2);

        //Mpu_1 change direction
        if (pitch_1 < -10){dir_1 = 1;} //mpu_1 go up
        if (pitch_1 > 10){dir_1 = 3;} //mpu_1 go down
        if (roll_1 > 10){dir_1 = 2;} //mpu_1 go right
        if (roll_1 < -10){dir_1 = 4;} //mpu_1 go left
        //Mpu_2 change direction
        if (pitch_2 < -10){dir_2 = 1;} //mpu_2 go up
        if (pitch_2 > 10){dir_2 = 3;} //mpu_2 go down
        if (roll_2 > 10){dir_2 = 2;} //mpu_2 go right
        if (roll_2 < -10){dir_2 = 4;} //mpu_2 go left

        // Connect to database
        conn = mysql_init(NULL);
        mysql_real_connect(conn,server,user,password,database,0,NULL,0);
        //Send data to database
        // Create sql command
        char sql_1[200];
        char sql_2[200];
        char sql_start_1[200];
        char sql_stop_1[200];
        char sql_start_2[200];
        char sql_stop_2[200];
        //Update status of button to database
        if  (flag == 1)
        {
            //sprintf(sql_start_1, "update control set start = 1 where id = 1");
            sprintf(sql_stop_1, "update control set stop = 0 where id = 1");
            //sprintf(sql_start_2, "update control set start = 1 where id = 2");
            //sprintf(sql_stop_2, "update control set stop = 0 where id = 2");
            //show(STOP); //show max7219 --STOP--
            for (int i = 1; i < 8; i++)
            {
                send_data(i, 0x0A);
            }
        }
        if  (flag == 0)
        {
            sprintf(sql_start_1, "update control set start = 1 where id = 1");
            sprintf(sql_stop_1, "update control set stop = 1 where id = 1");
            
            //show(PLAY); //show max7219 --PLAY--
        }
        //Update direction to database
        sprintf(sql_1,"update control set direction = (%d) where id = 1", dir_1);
        sprintf(sql_2,"update control set direction = (%d) where id = 2", dir_2);
        
        // send SQL query
        mysql_query(conn, sql_1);
        mysql_query(conn, sql_2);
        // send SQL query
        mysql_query(conn, sql_start_1);
        mysql_query(conn, sql_stop_1);

        //Receive data from database
        char sql[200];
        //Get score from database
        for (int i=1;i<3;i++)
        {
            sprintf(sql,"select * from control where id=%d", i);
            mysql_query(conn, sql);
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);
            if (i == 1) //snake 1
            {  
                for (int j=1;j<6;j++)
                {
                    snake_1[j-1]=atoi(row[j]);
                }
            }
            if (i == 2) //snake 2
            {  
                for (int j=1;j<6;j++)
                {
                    snake_2[j-1]=atoi(row[j]);
                }
            }
        }
        score_snake_1 = snake_1[1]; //Get data score frfom database
        score_snake_2 = snake_2[1];
        printf("Score 1: %d\n", score_snake_1); //Print score of 2 player on terminal
        printf("Score 2: %d\n", score_snake_2);
        showscore_1(score_snake_1); //Show score of 2 player on max7219
        showscore_2(score_snake_2);
        winner1 = snake_1[4]; //Get data winner from database
        winner2 = snake_2[4];
        if (winner1 == 1 && winner2 == 0){show(P1);} //Show player1 win the game
        if (winner1 == 0 && winner2 == 1){show(P2);} //Show player2 win the game
        if (winner1 == 1 && winner2 == 1){show(TIE);} //Show 2 players tied
        if (winner1 == 1 || winner2 == 1) 
        {
            flag = 2;
            sprintf(sql_start_2, "update control set start = 0 where id = 1");
            //sprintf(sql_stop_2, "update control set stop = 1 where id = 2");
        }
        mysql_query(conn, sql_start_2);
        mysql_query(conn, sql_stop_2);
        //Close connection
        mysql_free_result(res);
        mysql_close(conn);
       
    }
    return 0;
}
