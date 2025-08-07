#include "mbed.h"

BufferedSerial uart0(P1_7, P1_6);
I2C i2c(P0_5,P0_4);     // sda, scl

//lcd
const uint8_t lcd_addr=0x7C;   //lcd i2c addr 0x7C
void lcd_init(uint8_t addr);     //lcd init func
void char_disp(uint8_t addr, uint8_t position, char data);
char buf[9];
char pat[8]={0x0,0x24,0x4E,0x75,0x84,0xA4,0xC4,0xE4};   //up arrow character pattern

//ina236
const uint8_t ina_addr=0x80;   //ina236 i2c addr 0x40<<1
char w_buf[3];
char r_buf[2];
uint16_t vbus;  //mV unit
int16_t cur;    //mA unit
float vbus_f, v_shunt, i_f;      //mV, uV, mA unit

//constants
#define shunt_R 0.005
#define v_ofs 0
#define v_gain 1
#define c_ofs 0
#define c_gain 0.98668
#define c_ofs_com 13

int main(){
    i2c.frequency(100000);  //I2C clk 100kHz
    thread_sleep_for(100);  //wait for LCD power on
    lcd_init(lcd_addr);
    thread_sleep_for(100);

    char_disp(lcd_addr,7,'V');
    char_disp(lcd_addr,0x40+7,'A');

    //init ina236
    w_buf[0]=0;
    w_buf[1]=(0b10<<5)+(0b0<<4)+(0b011<<1)+0b1; //FS, AVG, VBUSCT
    w_buf[2]=(0b00<<6)+(0b100<<3)+0b111;        //VBUSCT, VSHCT, MODE
    i2c.write(ina_addr,w_buf,3);

    //set up arrow character
    buf[0]=0x00;
    buf[1]=0x40|0x00;
    i2c.write(lcd_addr,buf,2);
    thread_sleep_for(10);
    buf[0]=0x40;
    buf[1]=pat[0];
    buf[2]=pat[1];
    buf[3]=pat[2];
    buf[4]=pat[3];
    buf[5]=pat[4];
    buf[6]=pat[5];
    buf[7]=pat[6];
    buf[8]=pat[7];
    i2c.write(lcd_addr,buf,9);

    while (true){
        //read vbus
        w_buf[0]=0x2;
        i2c.write(ina_addr,w_buf,1);
        i2c.read(ina_addr|1,r_buf,2);
        vbus_f=(((r_buf[0]<<8)+r_buf[1])*1.6-v_ofs)*v_gain;    //1LSB=1.6mV, mV unit
        vbus=(uint16_t)vbus_f;

        //read current
        w_buf[0]=0x1;
        i2c.write(ina_addr,w_buf,1);
        i2c.read(ina_addr|1,r_buf,2);
        v_shunt=((int16_t)((r_buf[0]<<8)+r_buf[1])*2.5-c_ofs)*c_gain;    //1LSB=2.5uV, uV unit
        i_f=v_shunt/shunt_R/1000;   //mA unit
        cur=(int16_t)i_f;

        //V disp
        buf[0]=0x0;
        buf[1]=0x80+0x1;    //position
        i2c.write(lcd_addr,buf,2);
        buf[0]=0x40;
        buf[1]=0x30+(vbus/10000)%10;    //10
        buf[2]=0x30+(vbus/1000)%10;     //1
        buf[3]=0x2E;                    //.
        buf[4]=0x30+(vbus/100)%10;      //0.1
        buf[5]=0x30+(vbus/10)%10;      //0.01
        buf[6]=0x30+(vbus/1)%10;      //0.001
        i2c.write(lcd_addr,buf,7);

        //Cur disp
        if(cur<0){
            cur=abs(cur);
            cur=cur-c_ofs_com;
            char_disp(lcd_addr,0x40+0,0b00000000);  //up arrow
        }else{
            char_disp(lcd_addr,0x40+0,0b00000110);  //down arrow
        }
        
        buf[0]=0x0;
        buf[1]=0x80+0x42;    //position
        i2c.write(lcd_addr,buf,2);
        buf[0]=0x40;
        buf[1]=0x30+(cur/1000)%10;     //1
        buf[2]=0x2E;                    //.
        buf[3]=0x30+(cur/100)%10;      //0.1
        buf[4]=0x30+(cur/10)%10;      //0.01
        buf[5]=0x30+(cur/1)%10;      //0.001
        i2c.write(lcd_addr,buf,6);
    }
        
}

//disp char func
void char_disp(uint8_t addr, uint8_t position, char data){
    char buf[2];
    buf[0]=0x0;
    buf[1]=0x80+position;   //set cusor position (0x80 means cursor set cmd)
    i2c.write(addr,buf,2);
    buf[0]=0x40;            //write command
    buf[1]=data;
    i2c.write(addr,buf,2);
}

//LCD init func
void lcd_init(uint8_t addr){
    char lcd_data[2];
    lcd_data[0]=0x0;
    lcd_data[1]=0x38;
    i2c.write(addr,lcd_data,2);
    lcd_data[1]=0x39;
    i2c.write(addr,lcd_data,2);
    lcd_data[1]=0x14;
    i2c.write(addr,lcd_data,2);
    lcd_data[1]=0x70;
    i2c.write(addr,lcd_data,2);
    lcd_data[1]=0x56;
    i2c.write(addr,lcd_data,2);
    lcd_data[1]=0x6C;
    i2c.write(addr,lcd_data,2);
    thread_sleep_for(200);
    lcd_data[1]=0x38;
    i2c.write(addr,lcd_data,2);
    lcd_data[1]=0x0C;
    i2c.write(addr,lcd_data,2);
    lcd_data[1]=0x01;
    i2c.write(addr,lcd_data,2);
}
