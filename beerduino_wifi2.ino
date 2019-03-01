#include <SPI.h>
#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>
#include <OneWire.h>
#include <SD.h>
#include <PID_v1.h>
#include <EEPROM.h>
#include <Time.h>
#include <Wire.h>
#include <DS1307RTC.h>
//#include <max6675.h>
//#include <Servo.h>



#define END_CHAR '$'
#define START_CHAR '!'

//Servo servo;

//пид
double Setpoint, Input, Output;
PID myPID(&Input, &Output, &Setpoint,100,20,5, DIRECT);
unsigned long windowStartTime;

OneWire ds(10); // порт температурного датчика
//MAX6675 thermocouple(16, 17, 18);


extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];
extern uint8_t Dingbats1_XL[];
extern unsigned int bgr1[0x3A55];
extern unsigned int bgr2[0x10E0];


UTFT          myGLCD(SSD1289,38,39,40,41);
//UTFT          myGLCD(TFT01_32QVT,38,39,40,41);
// UTFT myGLCD(ITDB32WC, 38,39,40,41); 
//UTFT myGLCD(CTE32_R2,38,39,40,41);
//UTFT myGLCD(ILI9341_16,38,39,40,41);
//UTFT myGLCD(ILI9341_S5P, 38, 39, 40, 41);
//UTFT   myGLCD(ITDB32S,38,39,40,41);
//UTFT myGLCD(ITDB32WC,38,39,40,41); 
UTouch        myTouch(6,5,4,3,2);

UTFT_Buttons  myButtons(&myGLCD, &myTouch);

const int chipSelect = 53;
const int pin_relay=8;   // реле нагрева
const int pin_pump=12;   // реле помпы
const int pin_zvuk=9;    // пьезодинамик


const int pin_par=11;   // реле клапана отбора

const int pin_cool=13; 

const char *sensor[]={"INT","OUT"};
const char *onoff[]= {"OFF","ON"};


int nomer_pauzi=0; // номер паузы
float t_zatora=0 ; // температура затора и погрешность
boolean  nagrev;
byte bgcolor[]={44,108,205}; // цвет фона
unsigned long sp,vsp,vnv,vnz;
unsigned long pauza[6]; // задержка выполнения
int kol_receptov=0;
int nom_rec=0;
char recfile[30][13];
int kolfiles=0;
char  recname[30][21];
int   recpara[30][19];
int zaderjka=0;
int vdkz;

boolean SDin=false;              
char serialMessage[85];
unsigned int count;
boolean readingSerial;


int knkd=0;
int sledhop=0;
int dkpm=0;
int   nfile=0;
boolean pump_work=LOW;
boolean solod=false, yod=false;
boolean nkpz=false;
byte shim=100, ert=0;
unsigned long sump=0;
unsigned long vpop=0;
int npp=0;
int maxvp=100; // максимальное время паузы
  int dh=0;

  int kol_pauz=2, vr_varki=30, kol_hop=0, virpul=0;
  int  pressed_button;
  int zator, varka , minus, plus, dalee, nazad, plus_m[5], minus_m[5], recep, zater, varev, stng, moyka, vr_minus, tim, stp, ppm, pproc; // кнопки
  int menu=0, drowMenu=-1;
  int tp[6], vp[6],vh[5],st[15];

byte nstad=0;                   
byte beepwork=0;
int  beeptime=0;  
File opfile;
int l_etap;
unsigned long starttime=0;
boolean rtctf=false; //RTC
unsigned long dkvrtc=0;
tmElements_t tm;
int chas,minuta;



void setup()
{
  Serial3.begin(57600);
  Serial.begin(9600);
   
  pinMode(pin_relay, OUTPUT);
  pinMode(pin_pump,  OUTPUT);

  digitalWrite (pin_pump ,LOW);
  digitalWrite (pin_relay,LOW);

  analogWrite (pin_zvuk ,0);
 /* for (byte j=3; j <=7 ; j++) {  
  servo.attach(j);
  servo.write(0);
  delay(50); 
  }
  
*/  
  
  

  
  if (RTC.get()!=0) {//RTC
  rtctf=true; setSyncProvider(RTC.get);starttime=now();
  }
  myGLCD.InitLCD();
  myGLCD.setContrast(64);
//  myGLCD.setBackColor(VGA_TRANSPARENT);
  myGLCD.setBackColor(bgcolor[0],bgcolor[1],bgcolor[2]);
  myGLCD.fillScr(bgcolor[0],bgcolor[1],bgcolor[2]);
  myGLCD.setColor(255,255,255);
  myButtons.setTextFont(BigFont);
  myButtons.setSymbolFont(Dingbats1_XL);
  myButtons.setButtonColors(VGA_WHITE,VGA_GRAY,VGA_WHITE,VGA_RED,0x191970); // цвета кнопок
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
  myGLCD.setFont(SmallFont);
  if (!SD.begin(chipSelect)) {SDin=false; myGLCD.print("SD ""\x9F""ap""\xA4""a ""\xA2""e o""\x96\xA2""apy""\x9B""e""\xA2""a.", CENTER, 220);  } 
  else { SDin=true; scanSD();
  if (kolfiles<1) {myGLCD.print("\x8B""a""\x9E\xA0\xAB"" c pe""\xA6""e""\xA3\xA4""a""\xA1\x9D"" ""\xA2""e ""\xA2""a""\x9E\x99""e""\xA2\xAB"".", CENTER, 220); }
  else {recep = myButtons.addButton( 8, 170, 235,  60,"PE""\x8C""E""\x89""T""\x91");}
  }

  zator = myButtons.addButton( 8,  40, 235,  60, "\x85""AT""\x86""PAH""\x86""E"); // кнопка "ЗАТИРАНИЕ"
  varka = myButtons.addButton( 8,  105, 235,  60, "BAPKA");
  stng  = myButtons.addButton( 250,  40, 60,  60, "x", BUTTON_SYMBOL);
  moyka  = myButtons.addButton( 250,  105, 60,  60, "p", BUTTON_SYMBOL);
  tim    =myButtons.addButton( 250, 170, 60,  60, "W", BUTTON_SYMBOL); // кнопка "Часы"
  stp    =myButtons.addButton( 273, 70, 40,  40, "4", BUTTON_SYMBOL); // кнопка "Стоп"
  minus = myButtons.addButton( 50,  60, 70,  70, "-");
  plus  = myButtons.addButton( 200, 60, 70,  70, "+"); 
  dalee  =myButtons.addButton( 180,200, 130,  35, "\x82""A""\x88""EE >"); // кнопка "ДАЛЕЕ"
  nazad  =myButtons.addButton( 10, 200, 130,  35, "< HA""\x85""A""\x82"); // кнопка "НАЗАД"
  zater  =myButtons.addButton( 180, 160, 130,  35, "3ATEPET""\x92");     // кнопка "ЗАТЕРЕТЬ"
  varev  =myButtons.addButton( 10, 160, 130,  35, "BAP""\x86""T""\x92"); // кнопка "Варить"
  ppm  =myButtons.addButton( 273,120, 40, 40, "+5"); // кнопка "+5мин"
  pproc= myButtons.addButton(  7,120, 40, 40, "P"); // кнопка "пауза"
 
  for (int j=0; j <5 ; j++) {
  minus_m[j]  =myButtons.addButton( 50,4+38*j , 70,  30, "-");
  plus_m[j]  = myButtons.addButton( 200,4+38*j ,70,  30, "+");
  tp[j+1]=30+10*j;
  vp[j+1]=10;
  }
//  myGLCD.drawRoundRect (25,25,295,215);
  myGLCD.setFont(BigFont);
  myGLCD.drawBitmap (33,35, 137, 109, bgr1,1);
  myGLCD.drawBitmap (170,108, 120, 36, bgr2,1);
//  myGLCD.print ("BEERDUINO  v1.2",CENTER,50);

//  if (rtctf)   myGLCD.print ("RTC" ,5,5); // индикация RTC
// myGLCD.print ("M.""\x82\x83""M""\x86""H",175,45);
delay (1000);
  myGLCD.print ("3A""\x89""PE""\x8F""EHO" ,CENTER,160); 
delay (1000);
  myGLCD.print ("KOMMEP""\x8D""ECKOE",CENTER,180);
delay (1000);
  myGLCD.print ("\x86""C""\x89""O""\x88\x92""3OBAH""\x86""E",CENTER,200);
delay (1000);
 myGLCD.print ("creator M.""\x82\x83""M""\x86""H",CENTER,12);
l_etap=read_EEPROM(20);
if (l_etap==1 || l_etap==2) {drowMenu=0; menu=41;}

delay (800);
beeper();  


  
}



void loop()
{
if (menu!=0) {while (Serial3.available()) {Serial3.read();}}
if (clock() >= (pauza[0] + 100)){
tz();

senddatawifi ();
pauza[0] = clock();
}

if (clock() >= (pauza[4] + 60000) ){ //пишем лог  каждые 1 минут
logger ();
if (rtctf) {setSyncProvider(RTC.get);}//RTC  
pauza[4] = clock(); 
}

if (myTouch.dataAvailable())    {

 pressed_button = myButtons.checkButtons();  
  if      (pressed_button==zator)     {menu=1; }
  else if (pressed_button==varka)     {menu=10; }
  else if (pressed_button==recep)     {menu=20; }
  else if (pressed_button==stng)      {menu=30; }
  else if (pressed_button==zater)     {nom_rec=0;menu=4; }
  else if (pressed_button==varev)     {nom_rec=0;menu=13; }
  else if (pressed_button==moyka)     {menu=40;}
  else if (pressed_button==tim)       {menu=42;}
  else if (pressed_button==stp && knkd>1)  {menu=0; knkd=0;}  
  else if (pressed_button==stp)  {knkd++;}  
  else if (pressed_button==ppm) {sp=sp+300000;}
  else if (pressed_button==pproc && nkpz==false){nkpz=true; pump_work=LOW; digitalWrite(pin_relay,LOW);digitalWrite(pin_pump,LOW);}
  else if (pressed_button==pproc && nkpz==true){nkpz=false;}
  else if (pressed_button==plus) {

  if      (menu==1)     {kol_pauz++; kol_pauz=constrain(kol_pauz, 2, 5); myGLCD.printNumI(kol_pauz, CENTER, 70); }
  if      (menu==5)     {zaderjka=zaderjka+10; zaderjka=constrain(zaderjka, 0, 480); myGLCD.print(" "+String(zaderjka)+ " ", CENTER, 85); }
  else if (menu==11)    {kol_hop++;    kol_hop=constrain(kol_hop, 0, 5);  myGLCD.printNumI(kol_hop, CENTER,70 );}
  else if (menu==10)    {vr_varki=vr_varki+5;    vr_varki=constrain(vr_varki, 10, 900); myGLCD.print("   ", CENTER,90 );  myGLCD.printNumI(vr_varki, CENTER,90 );}  
  else if (menu==15)    {virpul=1;   myGLCD.print("   ", CENTER,90 );  myGLCD.print(onoff[virpul], CENTER,90 );}
  else if (menu==20)    {nfile++; nfile=constrain(nfile, 0, kolfiles-1);  myGLCD.print("                ", CENTER,150 );  myGLCD.print(recfile[nfile], CENTER,150 );}  
}
  else if (pressed_button==minus) {

  if      (menu==1)    {kol_pauz--; kol_pauz=constrain(kol_pauz, 2, 5); myGLCD.printNumI(kol_pauz, CENTER, 70);}
  if      (menu==5)    {zaderjka=zaderjka-10; zaderjka=constrain(zaderjka, 0, 480); myGLCD.print(" "+String(zaderjka)+ " ", CENTER, 85); }
  else if (menu==11)   {kol_hop--;   kol_hop=constrain(kol_hop, 0, 5);  myGLCD.printNumI(kol_hop, CENTER,70 );}  
  else if (menu==10)   {vr_varki=vr_varki-5; vr_varki=constrain(vr_varki, 10, 900); myGLCD.print("   ", CENTER,90 );  myGLCD.printNumI(vr_varki, CENTER,90 );}
  else if (menu==15)   {virpul=0;   myGLCD.print("   ", CENTER,90 ); myGLCD.print(onoff[virpul], CENTER,90 );}
  else if (menu==20)   {nfile--;  nfile=constrain(nfile, 0, kolfiles-1); myGLCD.print("                ", CENTER,150 ); myGLCD.print(recfile[nfile], CENTER,150 );}  
 }
  
  
  else if (pressed_button==dalee)   {
  if (menu==22 && nom_rec>=0)  {nom_rec++;drowMenu=1000;}    
  else if (menu==32 && st[9]==1)  {menu=34;}  
  else if (menu==5)  {vnz=clock(); menu=6; disableKnopok (drowMenu);} 
  else if (menu==11 && kol_hop<1)  {menu=13;} 
  else if (menu==15 && virpul==1)  {menu=16;} 
  else if (menu==15 && virpul==0)  {menu=0;} 
  else if (menu==16 || menu==40)  {menu=0;} 
  else if (menu==41) {
      sp=clock()+60000000;
      nagrev=true;
      load_settings ();
    
    if (l_etap==2) {
      vr_varki=read_EEPROM(21);
      kol_hop =read_EEPROM(22);
      for (int j=0; j <kol_hop; j++) {vh[j]=read_EEPROM(23+j);} 
      menu=14;
    }
    else if (l_etap==1) {
      int aa=0;
      nomer_pauzi=read_EEPROM(21);
      kol_pauz =read_EEPROM(22);
      solod=(read_EEPROM(23)==1) ? true : false;
      for (int j=nomer_pauzi; j <=kol_pauz; j++) {
      aa=j-nomer_pauzi; 
      tp[j]=read_EEPROM(24+aa); 
      }
      for (int j=nomer_pauzi; j <=kol_pauz; j++) {
      aa=aa+1;
      vp[j]=read_EEPROM(24+aa); 
      }
      menu=7;
    }
  }
  else if (menu==42) {
    tm.Day = 1;
    tm.Month = 1;
    tm.Year = CalendarYrToTm(2001);
    tm.Hour = chas;
    tm.Minute = minuta;
    tm.Second = 1;
    RTC.write(tm);
    delay(50);
    setSyncProvider(RTC.get);
    menu=0;
  }  
  else {menu++;}
}
  
  else if (pressed_button==nazad)   {
  if      (menu==1 || menu==10 || menu==20 || menu==30)   {menu=0;} 
  else if (menu==13 && kol_hop<1)  {menu=11;} 
  else if (menu==22 && nom_rec==0)  {menu=21;}    
  else if (menu==22 && nom_rec>0)   {nom_rec--;drowMenu=1000;}    
  else if (menu==41)  {menu=0;l_etap=0;} 
  else if (menu==42)  {menu=0;}
  else {menu--;}
  }

 
  else if (menu==2) {gradusi(pressed_button);}
  else if (menu==3) {vremya (pressed_button);}
  else if (menu==12) {hop(pressed_button);}
  else if (menu==14) {
   
    if       (pressed_button == plus_m[4])     {shim++; }
    else if  (pressed_button ==minus_m[4])     {shim--; }
    shim=constrain(shim, 1, 100);
    myGLCD.print("    ", CENTER,164 );
    myGLCD.print(String(shim), CENTER,164 );
    
  //  if       (pressed_button == plus_m[4])     {dh++; }
  //  else if  (pressed_button ==minus_m[4])     {dh--; }

//    dh=constrain(dh, -3, 3);
//    myGLCD.print("    ", CENTER,164 );
//    myGLCD.print(String(st[5]+dh), CENTER,164 );
  }
  else if (menu==31) {nastr(pressed_button, 5);}
  else if (menu==32) {nastr(pressed_button, 5);}
  else if (menu==33) {nastr(pressed_button, 5);}
  else if (menu==42){
    if (pressed_button==minus_m[0]) {chas--;}
    else if (pressed_button==plus_m[0]) {chas++;}
    else if (pressed_button==minus_m[1]){ minuta--;}
    else if (pressed_button==plus_m[1]) {minuta++;}
  }
  }

switch (menu) {
    
case 0: // главное меню 
if (drowMenu!=0) {
  vpop==0; nomer_pauzi=0;nstad=0;dkpm=0;solod=false; 
  drowMenu=0; 
  disableKnopok (drowMenu);
// myGLCD.print ("BEERDUINO",75,12);
  myButtons.enableButton (zator, true);
  myButtons.enableButton (varka, true);
  myButtons.enableButton (recep, true);
  myButtons.enableButton (stng, true);
  myButtons.enableButton (moyka, true);
  if (!rtctf) myGLCD.print ("BEERDUINO v1.5",75,12);  
  else   myButtons.enableButton (tim, true);
 
  sp=clock()+60000000;
  nagrev=true;
  load_settings ();
  digitalWrite (pin_relay,LOW);
  digitalWrite (pin_pump ,LOW);
  analogWrite (pin_zvuk,0);
  write_EEPROM(20,0);
}
if (rtctf){myGLCD.print (nullvp(hour())+":"+nullvp(minute()),75,12);}
   if (Serial3.available() > 0 &&  !readingSerial) {
    if (Serial3.read() == START_CHAR) {
      serialRead();
    }
  }
  
myGLCD.print (String((int)t_zatora) + "\x7F" , 10 ,12);
break; 

// ------------------------------------------------------Затирание--------------------------------------------

case 1: // количество пауз
if (drowMenu==1) break;
  disableKnopok (drowMenu);
  bpm();
  drowMenu=1;
  myGLCD.print ("KO""\x88\x86\x8D""ECTBO ""\x89""A""\x8A\x85",CENTER,10);
  myGLCD.setFont(SevenSegNumFont);
  myGLCD.printNumI(kol_pauz, CENTER, 70);


break;

case 2:  // температура пауз
if (drowMenu==2) break;
  disableKnopok (drowMenu);
  mpm(kol_pauz);
  drowMenu=2;
  myGLCD.setFont(SmallFont);
  myGLCD.print ("TEM""\x89""EPAT""\x8A""PA ""\x89""A""\x8A\x85",15,25,90);
  myGLCD.setFont(BigFont);
  for (int j=0; j <kol_pauz ; j++) {
  myGLCD.print(String(tp[j+1])+"\x7F", CENTER,12+38*j );
}

break;

case 3: // время пауз
if (drowMenu==3) break;
  disableKnopok (drowMenu);
  mpm(kol_pauz);
  drowMenu=3;
  myGLCD.print ("BPEM""\x95"" ""\x89""A""\x8A\x85",20,10,90);
   for (int j=0; j <kol_pauz ; j++) {
   myGLCD.printNumI(vp[j+1], CENTER,12+38*j );}


break;
case 4:
if (drowMenu==4) break; // проверка
  disableKnopok (drowMenu);
  myButtons.enableButton (dalee, true); 
  myButtons.enableButton (nazad, true); 
  myGLCD.print ("\x89""POBEPKA" , CENTER ,10);
  tp[0]=tp[1]+st[7];
  vp[0]=10;
  myGLCD.setFont(SmallFont);
  grafik ();
  for (int j=1; j <=kol_pauz ; j++) {
  myGLCD.print ("\x89""A""\x8A\x85""A N"+String(j)+": "+"T="+String(tp[j])+"\x7F | B="+String(vp[j])+"\xA1\x9D\xA2"  , CENTER ,20+15*j);
  }
  drowMenu=4;

break;
case 5:
if (drowMenu==5) break;
  disableKnopok (drowMenu);
  bpm();
  drowMenu=5;
  myGLCD.print ("3A""\x82""EP""\x84""KA",CENTER,10);
  myGLCD.printNumI(zaderjka, CENTER, 85);
break;

case 6:
  vdkz=zaderjka*60 -(clock()-vnz)/1000;
  if (vdkz<1 || zaderjka==0) {menu=7;break;}

if (drowMenu!=6) {
   disableKnopok (drowMenu);
  myButtons.enableButton (dalee, true); 
  myButtons.enableButton (nazad, true); 
  myGLCD.print ("CTAPT ""\x8D""EPE3",CENTER,10);
  myGLCD.print("CEK", CENTER, 155);
  myGLCD.setFont(SevenSegNumFont);
  drowMenu=6;
}

  myGLCD.printNumI( vdkz, CENTER, 85);

break;

case 7: // процесс затирания

if (drowMenu!=7) {
  nstad=1;
  disableKnopok (drowMenu);
  myGLCD.print ("3AT""\x86""PAH""\x86""E" , CENTER ,15);
  oxi_pump ();
  myGLCD.print ("TEM""\x89""EPAT""\x8A""PA 3ATOPA" , CENTER ,40);
  drowMenu=7;
  windowStartTime = clock();

}
if (nkpz==false) pump_zator();
pumpind();
progon_po_pauzam();

break;
//-------------------------------------Варка------------------------------------------------

case 10: // время варки
if ( drowMenu==10) break;
  disableKnopok (drowMenu);
  bpm(); 
  drowMenu=10;
  myGLCD.print ("BPEM""\x95"" BAPK""\x86",CENTER,10);
  myGLCD.printNumI(vr_varki, CENTER,90 );


break;

case 11: // количество хмеля
if (drowMenu==11) break;
  disableKnopok (drowMenu);
  bpm();
  drowMenu=11;
  myGLCD.print ("KO""\x88\x86\x8D""ECTBO XME""\x88\x95",CENTER,10);
  myGLCD.setFont(SevenSegNumFont);
  myGLCD.printNumI(kol_hop, CENTER,70 );


break;

case 12: // время закладки хмеля
if (drowMenu==12) break;
  disableKnopok (drowMenu);
  mpm(kol_hop);
  drowMenu=12;

  myGLCD.print ("BPEM""\x95"" XME""\x88\x95" ,20,10,90);
   for (int j=0; j <kol_hop ; j++) {
   vh[j]=vr_varki-2*j;
   myGLCD.printNumI(vh[j], CENTER,12+38*j );}


break;

case 13:// проверка
if (drowMenu==13) break; 
  disableKnopok (drowMenu);
  myButtons.enableButton (dalee, true); 
  myButtons.enableButton (nazad, true); 
  myGLCD.print ("\x89""POBEPKA" , CENTER ,10);
  myGLCD.setFont(SmallFont);
  myGLCD.print ( "BAPKA " +  String(vr_varki)+"\xA1\x9D\xA2", CENTER ,33);
  myGLCD.drawRect (10,170,310,171);
  myGLCD.print ("0\xA1\x9D\xA2" ,10,175);
  myGLCD.print (String(vr_varki)+"\xA1\x9D\xA2" ,270,175);

  for (int j=0; j <kol_hop ; j++) {
  myGLCD.print ("XME""\x88\x92"" N"+String(j+1)+ "  "+String(vh[j])+" \xA1\x9D\xA2", CENTER ,50+15*j);
  float hopnsh=10+300*((float)(vr_varki-vh[j])/(float)vr_varki);
  myGLCD.drawCircle(hopnsh,170,3);
  myGLCD.printNumI ( j+1  ,hopnsh ,150);
  }
  
  drowMenu=13;


break;

case 14: // процесс варки
if (drowMenu!=14) {
  nstad=2;
  disableKnopok (drowMenu);
  myButtons.enableButton (stp, true); 
  myGLCD.print ("BAPKA" , CENTER ,15);
  if (st[11]==1)  oxi_pump ();
  myGLCD.print ("TEM""\x89""EPAT""\x8A""PA C""\x8A""C""\x88""A" , CENTER ,40);
  myButtons.enableButton (minus_m[4], true);
  myButtons.enableButton (plus_m[4], true);
  myGLCD.print(String(shim), CENTER,164 );
  drowMenu=14;
  nagrev=true;
  windowStartTime = clock();

}
pump_varka();
pumpind();
varka_susla();

break;

case 15: // вирпул
if (drowMenu==15) break;
  disableKnopok (drowMenu);
  myButtons.enableButton (plus, true);  
  myButtons.enableButton (minus, true); 
  myButtons.enableButton (dalee, true);
  drowMenu=15;
  myGLCD.print ("B""\x86""P""\x89\x8A\x88",CENTER,10); //ВИПРПУЛ
  myGLCD.print(onoff[virpul], CENTER,90 );


break;

case 16: // вирпул
if (drowMenu!=16) {
  disableKnopok (drowMenu);
  myButtons.enableButton (dalee, true);
  drowMenu=16;
  myGLCD.print ("B""\x86""P""\x89\x8A\x88",CENTER,10);


}

myGLCD.setFont(BigFont);
if (t_zatora<=st[8] && t_zatora>st[8]-0.1){beeper();  myGLCD.print ("\x89""OPA C""\x88\x86""BAT""\x92",CENTER,170);}



if (t_zatora<st[10] )  {digitalWrite (pin_pump ,HIGH);
myGLCD.print (" PA""\x80""OTAET ""\x89""OM""\x89""A  ",CENTER,150);
}
else {digitalWrite (pin_pump ,LOW);
myGLCD.print ("\x89""OM""\x89""A OCTAHOB""\x88""EHA",CENTER,150);
}
myGLCD.setFont(SevenSegNumFont);
myGLCD.print ( "      " ,CENTER,65);
myGLCD.printNumF ( t_zatora, 1 ,CENTER,65);
break;


// ----------------------------------Рецепты------------------------------
case 20:
if (drowMenu==20) break; 
  disableKnopok (drowMenu);
  bpm();
  drowMenu=20;
  myGLCD.print ("\x8B""A""\x87\x88\x91"" C PE""\x8C""E""\x89""TAM""\x86" , CENTER ,5);
  myGLCD.print(recfile[nfile], CENTER,150 );

break;

case 21:
if ( drowMenu==21) break; 
  Load_recepies(recfile[nfile]);
  disableKnopok (drowMenu);
  myButtons.enableButton (dalee, true); 
  myButtons.enableButton (nazad, true); 
  myGLCD.print ("PE""\x8C""E""\x89""T""\x91" , CENTER ,5);

 
  myGLCD.setFont(SmallFont);
  myGLCD.print ("\x8B""A""\x87\x88:"+String(recfile[nfile])+" : PE""\x8C""E""\x89""TOB  - "+String(kol_receptov)  , CENTER ,25);
//  myGLCD.setFont(BigFont);
int w;
w=(kol_receptov>10) ? 10 : kol_receptov;

   for (int j=0; j <w ; j++) {
String ukname=(String(recname[j]).length()>13)? String(recname[j]).substring(0, 13)+"\xB0" : recname[j];
  myGLCD.print ( ukname , 10 ,40+16*j);
  myGLCD.print ( "\x89""A""\x8A""3:"+String(recpara[j][1]), 133 ,40+16*j);
  myGLCD.print ( "| BAPKA:"+String(recpara[j][recpara[j][1]*2+2])+" \xA1\x9D\xA2",  190 ,40+16*j);

}

  drowMenu=21;
  

break;
 
case 22: 
if (drowMenu!=22) {
  boolean error=false;
  disableKnopok (drowMenu);
if (nom_rec<kol_receptov-1) {
  myButtons.enableButton (dalee, true); 
}
  myButtons.enableButton (nazad, true);
 
  myGLCD.print (recname[nom_rec] , CENTER ,10);
  myGLCD.setFont(SmallFont);
  kol_pauz=recpara[nom_rec][1];
  if (kol_pauz>5 || kol_pauz<2)  error=true;
  for (int j=2; j <= kol_pauz+1 ; j++) {
  tp[j-1]= recpara[nom_rec][j];
  vp[j-1]= recpara[nom_rec][j+kol_pauz];
  if  (tp[j-1]>85  || tp[j-1]<30) error=true;
  if  (vp[j-1]>100 || vp[j-1]<0)  error=true;
  myGLCD.print ("\x89""A""\x8A\x85""A N"+String(j-1)+": "+"T="+String(tp[j-1])+"\x7F | B="+String(vp[j-1])+"\xA1\x9D\xA2"  , CENTER ,14*j);
  }

  vr_varki=recpara[nom_rec][kol_pauz*2+2];   myGLCD.print ( "BAPKA " +  String(vr_varki)+"\xA1\x9D\xA2", CENTER ,100);
  if (vr_varki>150 || vr_varki<30)    error=true;
  kol_hop=recpara[nom_rec][kol_pauz*2+3];
  if (kol_hop>5 || kol_hop<0)       error=true;
  int x=0 , iks, igr;

  for (int j=0; j < kol_hop; j++) {
  vh[j] = recpara[nom_rec][kol_pauz*2+4+j];
  if  (vh[j]>vr_varki || vh[j]<0) error=true;
  if (x>0) {iks=178; igr=115+7*(j-1); x=0;}
  else {iks=33; igr=115+7*j; x++;}
  myGLCD.print ("XME""\x88\x92"" N"+String(j+1)+ " "+String(vh[j])+"\xA1\x9D\xA2", iks ,igr);
 
  }
if (!error) {myButtons.enableButton (varev, true); myButtons.enableButton (zater, true);}
else { myGLCD.setFont(BigFont);myGLCD.print ("O""\x8E\x86\x80""KA B PE""\x8C""E""\x89""TE",CENTER,160);} 

drowMenu=22;
}
break; 
//---------------------------------Настройки-------------------------------------
case 30:
if (drowMenu==30) break;
  disableKnopok (drowMenu);
  drowMenu=30;
  myButtons.enableButton (dalee, true); 
  myButtons.enableButton (nazad, true); 
  myGLCD.setFont(SmallFont);
  myGLCD.print ("Kp,Ki,Kd,WS - ""\xA3""apa""\xA1""e""\xA4""p""\xAB"" ""\x89\x86\x82",10,15);
  myGLCD.print ("pT - k""a""\xA0\x9D\x96""po""\x97""ka ""\xA4""ep""\xA1""o""\xA1""e""\xA4""pa",10,30);
  myGLCD.print ("Bo - ""\xA4""e""\xA1\xA3""epa""\xA4""ypa ""\x9F\x9D\xA3""e""\xA2\x9D\xAF",10,45);
  myGLCD.print ("PP - ""\xA3""po""\x99""y""\x97\x9F""a ""\xA3""o""\xA1\xA3\xAB",10,60);
  myGLCD.print ("DH - ""\x99""e""\xA0\xAC\xA4""a o""\xA4\xA2""oc""\x9D\xA4""e""\xA0\xAC\xA2""o 1""\x9E"" ""\xA3""ay""\x9C\xAB",10,75);
  myGLCD.print ("TV - ""\xA4""e""\xA1\xA3""epa""\xA4""ypa c""\xA0\x9D\x97""a",10,90);
  myGLCD.print ("Se - ""\xA1""ec""\xA4""o yc""\xA4""a""\xA2""o""\x97\x9F\x9D"" ""\xA4""ep""\xA1""o""\xA1""e""\xA4""pa",10,105);
  myGLCD.print ("PS - ""\xA4""e""\xA1\xA3""epa""\xA4""ypa oc""\xA4""a""\xA2""o""\x97\x9F\x9D"" ""\xA3""o""\xA1\xA3\xAB",10,120);
  myGLCD.print ("DB - ""\xA3""o""\xA1\xA3""a ""\x97""o ""\x97""pe""\xA1\xAF"" ""\x97""ap""\x9F\x9D",10,135);
  myGLCD.print ("PH - ""pe""\x9B\x9D\xA1\xAB"" ""\xA3""o""\xA1\xA3\xAB",10,150);
  myGLCD.print ("PC - ""\x97""pe""\xA1\xAF"" pa""\x96""o""\xA4\xAB"" ""\xA3""o""\xA1\xA3\xAB",10,165);
  myGLCD.print ("PR - ""\x97""pe""\xA1\xAF"" o""\xA4\x99\xAB""xa ""\xA3""o""\xA1\xA3\xAB",10,180);  


break;

case 31:
if (drowMenu!=31) {
  disableKnopok (drowMenu);
  mpm(5);
  drowMenu=31;

   myGLCD.print ("HACTPO""\x87""K""\x86"" 1" ,20,10,90);
   myGLCD.print("Kp", 275,12 );
   myGLCD.print("Ki", 275,50 );
   myGLCD.print("Kd", 275,88 );
   myGLCD.print("WS", 275,126 );
   myGLCD.print("pT", 275,164 );
}
   for (int j=0; j <5 ; j++) {
   if (j<3) st[j]=constrain(st[j], -150, 150); 
   st[3]=constrain(st[3], 0, 7500);
   st[4]=constrain(st[4], -5, 5);
   myGLCD.print(String(st[j]), CENTER,12+38*j );
   }
break;
case 32:
if (drowMenu!=32) {
  disableKnopok (drowMenu);
  mpm(5);
  drowMenu=32;

   myGLCD.print ("HACTPO""\x87""K""\x86"" 2" ,20,10,90);
   myGLCD.print("Bo", 275,12 ); // Температура кипения
   myGLCD.print("PP", 275,50 ); // Продувка помпы
   myGLCD.print("DH", 275,88 ); // Дельта нагрева перед 1-й паузой
   myGLCD.print("TV", 275,126 ); // Температур сигнала при вирпуле
   myGLCD.print("Se", 275,164 ); //Сенсор внешний/внутренний
}

   st[5]=constrain(st[5], 80, 100);
   st[6]=constrain(st[6], 0, 1); 
   st[7]=constrain(st[7], -25, 25); 
   st[8]=constrain(st[8], 20, 30); 
   st[9]=constrain(st[9], 0, 1); 
   
   myGLCD.print(String(st[5]), CENTER,12 );
   myGLCD.print(onoff[st[6]], CENTER,50 );
   myGLCD.print(String(st[7]), CENTER,88 );
   myGLCD.print(String(st[8]), CENTER,126 );
   myGLCD.print(sensor[st[9]], CENTER,164 );
break;
case 33:
if (drowMenu!=33) {
  disableKnopok (drowMenu);
  mpm(5);
  drowMenu=33;

   myGLCD.print ("HACTPO""\x87""K""\x86"" 3" ,20,10,90);
   myGLCD.print("PS", 275,12 ); //Остановка помпы
   myGLCD.print("DB", 275,50 ); //Помпа во время кипячения
   myGLCD.print("PH", 275,88 ); //Помпа во время работы ТЭНа
   myGLCD.print("PC", 275,126 ); //Время работы насоса 
   myGLCD.print("PR", 275,164 ); //Время паузы
}

   st[10]=constrain(st[10], 80, 102);
   st[11]=constrain(st[11], 0, 1); 
   st[12]=constrain(st[12], 0, 1);
   st[13]=constrain(st[13], 0, 900);
   st[14]=constrain(st[14], 0, 900); 
   myGLCD.print(String(st[10]), CENTER,12 );
   myGLCD.print(onoff[st[11]], CENTER,50 );
   myGLCD.print(onoff[st[12]], CENTER,88 );
   myGLCD.print(String(st[13]), CENTER,126 );
   myGLCD.print(String(st[14]), CENTER,164 );
break;
case 34:
menu=0;
disableKnopok (drowMenu);
myGLCD.print ("COXPAHEHO" ,CENTER , 110);
upload_settings ();
break;

//-------------------Мойка оборудования----------------------
case 40:
if (drowMenu!=40) {
drowMenu=40;
disableKnopok (drowMenu);
myGLCD.print ("\x89""POM""\x91""BKA C""\x86""CTEM""\x91",CENTER,5);
oxi_pump ();
myButtons.enableButton (dalee, true);
}
 SSColor ();
if (t_zatora<st[10] )  {digitalWrite (pin_pump ,HIGH);
myGLCD.print (" PA""\x80""OTAET ""\x89""OM""\x89""A  ",CENTER,145);
}
else {digitalWrite (pin_pump ,LOW);
myGLCD.print ("\x89""OM""\x89""A OCTAHOB""\x88""EHA",CENTER,145);
}
break; 
//-----------------аварийная страница-------------------------
case 41:
if (drowMenu==41) break;
drowMenu=41;
disableKnopok (drowMenu);
myButtons.enableButton (dalee, true); 
myButtons.enableButton (nazad, true); 
myGLCD.print ("C""\x80""O""\x87"" ""\x89\x86""TAH""\x86\x95",CENTER,80);
break;

//--------------------Настройки Времени —---------------------------
case 42:
if (drowMenu!=42) {
drowMenu=42;
disableKnopok (drowMenu);
chas=hour(); minuta=minute(); 
mpm(2);


myGLCD.print ("BPEM""\x95" ,20,10,90);
myGLCD.print("\x8D", 275,12 );
myGLCD.print("M", 275,50 );
}
chas=constrain(chas, 0, 23);
myGLCD.print(nullvp(chas), CENTER,12 );
minuta=constrain(minuta, 0, 59);
myGLCD.print(nullvp(minuta), CENTER,50 );

break;



}
}
// ----------------------------------Функции------------------------------
void(* resetFunc) (void) = 0;

void disableKnopok (int nm){
if (nm>=0){
for (int j=0; j <30; j++) { myButtons.disableButton(j);}}
myGLCD.fillScr(bgcolor[0],bgcolor[1],bgcolor[2]);
myGLCD.setFont(BigFont);
}



void bpm() {
  myButtons.enableButton (plus, true);  
  myButtons.enableButton (minus, true); 
  myButtons.enableButton (dalee, true); 
  myButtons.enableButton (nazad, true);   
 
}

void mpm(int kk) {

  for (int j=0; j <kk ; j++) {
  myButtons.enableButton (minus_m[j], true);
  myButtons.enableButton (plus_m[j], true);
  }
  myButtons.enableButton (dalee, true); 
  myButtons.enableButton (nazad, true); 
    
  
}


void grafik () {
  int xnl[6];
  int ynl[6];
  int xkl[6];
  int ykl[6];
  int smper;
  int smpos;
  xkl[0]=0;
  myGLCD.print ("t=" + String((int)t_zatora) + "\x7F""C" , 20 ,170);
  myGLCD.drawLineBig (13,5,13,185,3); // ось Y
  myGLCD.drawLineBig (15,185,315,185,3); // ось x
  myGLCD.print ("t,""\x7F""C",30,5,90);
  myGLCD.print ("B,""\xA1\x9D\xA2",280,170);
  myGLCD.setColor(255,0,0);
  for (int j=1; j <=kol_pauz ; j++) {
  
  xnl[j]=5+xkl[j-1]+10;
  ynl[j]=195-tp[j];
  xkl[j]=xnl[j]+vp[j];
  ykl[j]=195-tp[j];
  myGLCD.drawLineBig (xnl[j],ynl[j],xkl[j],ykl[j],3);   
  }
  for (int j=2; j <=kol_pauz ; j++) {  myGLCD.drawLineBig (xkl[j-1],ykl[j-1],xnl[j],ynl[j],4);  }
  myGLCD.setColor(255,255,255);
  }


void hop(int pbn){
int nm=-1;
for (int j = 0; j <kol_hop; j++) { 
if (pbn==minus_m[j] || pbn==plus_m[j]) {nm=j;}
} 
if (nm==-1) return;

if (pressed_button==minus_m[nm]) {
vh[nm]=vh[nm]-1; 
if ( nm!=kol_hop-1 && vh[nm]<=vh[nm+1]) {vh[nm]=vh[nm]+1;}

myGLCD.print("  ", CENTER,12+nm*38);
}
if (pressed_button==plus_m[nm] ) {
vh[nm]=vh[nm]+1; 

if ( nm!=0 && vh[nm]>=vh[nm-1]) {vh[nm]=vh[nm]-1;}

myGLCD.print("  ", CENTER,12+nm*38);}

vh[nm]=constrain(vh[nm], 0, vr_varki); // ограничиваем время закладки хмеля 
myGLCD.printNumI(vh[nm], CENTER,12+38*nm );
}


void gradusi(int pbn){
  int nm=-1;
  for (int j = 0; j <kol_pauz; j++) {          
  if     (pbn==minus_m[j]) {nm=j+1; tp[nm]--;}
  else if (pbn==plus_m[j]) {nm=j+1; tp[nm]++;}
  }
  if (nm==-1) return;
  tp[nm]=constrain(tp[nm], 30, 85);  // ограничиваем температуру пауз 
  myGLCD.print(String(tp[nm])+"\x7F", CENTER,12+38*(nm-1) );
}
void vremya(int pbn){
  int nm=-1;
  for (int j = 0; j <kol_pauz; j++) {          
  if     (pbn==minus_m[j]) {nm=j+1; vp[nm]--;}
  else if (pbn==plus_m[j]) {nm=j+1; vp[nm]++;}
  }
  if (nm==-1) return;
   myGLCD.print("   ", CENTER,12+(nm-1)*38);
   vp[nm]=constrain(vp[nm], 1, maxvp);  // ограничиваем время пауз 
   myGLCD.printNumI(vp[nm], CENTER,12+38*(nm-1) );
}





void nastr(int pbn , int kstr){
int nmd;
int nm;
  for (int j = 0; j <kstr; j++) {          
  if     (pbn==minus_m[j] || pbn==plus_m[j]) nmd=j;
  }
  if      (menu==31) nm=nmd;
  else if (menu==32) nm=nmd+5;
  else if (menu==33) nm=nmd+10;

if (pressed_button==minus_m[nmd])     {
  if (nm==3)  st[3]=st[3]-100;
  else if (nm>=13) st[nm]=st[nm]-10;
  else  st[nm]=st[nm]-1;
  myGLCD.print("    ", CENTER,12+nmd*38);
}
if (pressed_button==plus_m[nmd] )     {
  if (nm==3)  st[3]=st[3]+100;
  else if (nm>=13) st[nm]=st[nm]+10;
  else st[nm]=st[nm]+1;  
  myGLCD.print("    ", CENTER,12+nmd*38);
}
}

void beeper() {
beepwork=1;
for (int j=1; j <=10 ; j++) {
analogWrite(pin_zvuk,155);
delay(50);
analogWrite(pin_zvuk,0);
delay(50);
}
}


void progon_po_pauzam(){
boolean beeperplay=false;
if (nagrev==false &&  clock()< vsp+3000) { beeper();} 

dkpm=(sp-clock())/60000;
int dkps=((sp-clock())/1000)-dkpm*60;


if(clock() >= (pauza[2] + 1000)){
 SSColor(); 
myGLCD.print ("-> "+String(tp[nomer_pauzi])+"\x7F""C",CENTER,125);
myGLCD.print ("   ""\x89""A""\x8A""3A N"+String(nomer_pauzi)+"   ",CENTER,150);
if (nagrev==false && dkpm<=maxvp) { myGLCD.print ("KOHE""\x8C"" ""\x89""A""\x8A""3""\x91""->"+nullvp(dkpm)+":"+nullvp(dkps)+"  ",10,200);}
else {myGLCD.print ("                    ",10,200); }
pauza[2] = clock();}



if (clock() >= (pauza[1] + 100)){

if (nkpz==false) PID_HEAT (tp[nomer_pauzi]);

if (nomer_pauzi>kol_pauz) {digitalWrite(pin_pump,LOW); digitalWrite(pin_relay,LOW);disableKnopok(1);myGLCD.print ("KOHE""\x8C"" 3AT""\x86""PAH""\x86\x95",CENTER,110); beeper(); delay (10000); menu=0; } 

/*Засыпка солода*/
if (nagrev==true && t_zatora>=tp[0] && solod==false){
myButtons.enableButton (dalee, true); 
digitalWrite(pin_pump, LOW); 
myGLCD.print ("3AC""\x91\x89\x92""TE CO""\x88""O""\x82",CENTER,150);

while(solod==false){
tz();
pumpind();
senddatawifi();
SSColor ();
//PID_HEAT (tp[1]);


if (myTouch.dataAvailable()) {if (myButtons.checkButtons()==dalee)   {myButtons.enableButton (stp, true);myButtons.enableButton (pproc, true);  sump=0; npp=0; solod=true;nomer_pauzi=1;nagrev=true;sp=clock()+6000000;}}
else if (!beeperplay){beeper(); beeperplay=true;}

}
myButtons.disableDraw (dalee);
myGLCD.setFont(BigFont);
}

if (nagrev==true && t_zatora>=tp[nomer_pauzi] && nomer_pauzi>0) {sp=clock()+long(vp[nomer_pauzi])*60000; nagrev=false; vsp=clock();}

if (clock()>= sp && nomer_pauzi>0) {
nomer_pauzi++;nagrev=true;sp=clock()+6000000; 
if (yod==true){
myButtons.disableButton(ppm);
myGLCD.print ("                    ",10,175);

}
} 

if (nomer_pauzi==kol_pauz-1 && nagrev==false && yod==false) {
if (sp-clock() <300000) {myButtons.enableButton (ppm, true);  beeper(); yod=true; myGLCD.print ("\x87""O""\x82""HA""\x95"" ""\x89""PO""\x80""A",CENTER,175);}
}



pauza[1] = clock();}

}




void PID_HEAT (int temperatura){
Input = t_zatora;
Setpoint = temperatura;
if ((st[0]==0 && st[1]==0 && st[2]==0) || st[3]==0) {
if (Input<Setpoint) digitalWrite(pin_relay,HIGH);
else digitalWrite(pin_relay,LOW);
}
else {  
  if((Setpoint - Input)>5){
    digitalWrite(pin_relay,HIGH);
    if ((Setpoint - Input)<6)
    {
      myPID.Compute();
    }
  }
  else{
    myPID.Compute();
    unsigned long now = clock();
    if(now - windowStartTime>st[3])
    {                                     //time to shift the Relay Window
      windowStartTime += st[3];
    }
    if((Output*(st[3]/100)) > now - windowStartTime)  digitalWrite(pin_relay,HIGH);
    else  digitalWrite(pin_relay,LOW);
  }
}
}





void varka_susla(){
dkpm=(sp-clock())/60000;
int dkps=((sp-clock())/1000)-dkpm*60;
if(clock() >= (pauza[2] + 1000)){
SSColor(); 
if (nagrev==false) {
myGLCD.print ("KOHE""\x8C"" BAPK""\x86""->"+nullvp(dkpm)+":"+nullvp(dkps)+"  ",10,125);
myGLCD.print ("                    ",CENTER,200);
}

pauza[2]= clock();}

//if (nagrev==false) {
//int ppw[2];
//ppw[0] = map(shim, 0, 100, 0, 5000);
//ppw[1] = 5000-ppw[0];
//if (millis()>pauza[5]) {pauza[5]=millis()+long(ppw[ert]);ert++;}
//if (ert>1 && ppw[1]>0) {ert=0; digitalWrite(pin_relay,LOW);}
//else if (ert==1) {digitalWrite(pin_relay,HIGH);} 
//} else digitalWrite(pin_relay,HIGH);




if (nagrev==false) { 
if (shim<100 && shim>0) { 
int ppw[2]; 
ppw[0] = map(shim, 0, 100, 0, 5000); 
ppw[1] = 5000-ppw[0]; 
if (millis()>pauza[5]) {pauza[5]=millis()+long(ppw[ert]);ert++;} 
if (ert>1) {ert=0; digitalWrite(pin_relay,LOW);} 
else if (ert==1) {digitalWrite(pin_relay,HIGH);} 
} 
else if (shim==0) {digitalWrite(pin_relay,LOW);ert=0;} 
else {digitalWrite(pin_relay,HIGH);ert=0;} 
} 
else digitalWrite(pin_relay,HIGH);

if (t_zatora>78.4 && t_zatora<78.6) digitalWrite(pin_par,HIGH);
else digitalWrite(pin_par,LOW);


if (clock() >= (pauza[1] + 100)){
 
if (nagrev==true && t_zatora>=st[5]) {nagrev=false; sp=clock()+long(vr_varki)*60000;   vnv=clock();}



//PID_HEAT (st[5]+dh);

if (clock()>=sp) {digitalWrite(pin_pump,LOW); digitalWrite(pin_relay,LOW);digitalWrite(pin_cool,HIGH);disableKnopok(1);
myGLCD.print ("KOHE""\x8C"" BAPK""\x86",CENTER,110);beeper();delay (3000);  menu=15;}

if (nagrev==false){
for (int j=0; j <kol_hop; j++) {
//if (clock()>vnv+long((float(vh[j])-0.25)*60000) && clock()<vnv+long((float(vh[j])+0.25)*60000)) 
if (clock()>sp-long((float(vh[j])+0.15)*60000) && clock()<sp-long((float(vh[j])-0.15)*60000)){ myGLCD.print ("3AC""\x91\x89\x92""TE XME""\x88\x92"" N" + String(j+1),CENTER,200); sledhop=j+1; /* srv_hop(j);*/   }
}

if (clock()>vnv+long((float(vr_varki)-10.12)*60000) && clock()<vnv+long((float(vr_varki)-9.88)*60000) ){beeper(); myGLCD.print ("BCTAB""\x92""TE ""\x8D\x86\x88\x88""EP.",CENTER,200);}
}

pauza[1]= clock();}


}









void tz() {
//  if (thermocouple.readCelsius()!=-100) {t_zatora=thermocouple.readCelsius()+(float)st[4]; return;}

  byte addr[8];
  if ( !ds.search(addr)) {ds.reset_search(); return; }
  if (OneWire::crc8(addr, 7) != addr[7]) {return;}
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  float celsius;
  switch (addr[0]) {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        
  
  
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         

  for ( i = 0; i < 9; i++) {          
    data[i] = ds.read();
  }
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; 
    if (data[7] == 0x10) {

      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
 
    if (cfg == 0x00) raw = raw & ~7;  
    else if (cfg == 0x20) raw = raw & ~3; 
    else if (cfg == 0x40) raw = raw & ~1; 
 
  }
  t_zatora = ((float)raw / 16.0)+ (float)st[4];
 
}




void load_settings () {
myPID.SetMode(AUTOMATIC);
for (int j=0; j <15 ; j++) {
st[j] = read_EEPROM (j);
}
myPID.SetTunings(st[0],st[1]/100,st[2]);
myPID.SetOutputLimits(0, 100);
myPID.SetSampleTime(5000);

}

void upload_settings () {
  for (int j=0; j <15 ; j++) {write_EEPROM (j,st[j]);}
}

void oxi_pump () {
if (l_etap>0) return;
if (st[6]==1 && t_zatora<=st[10]) {
  myGLCD.print ("\x89""POKA""\x8D""KA HACOCA" , CENTER ,40); 
 for (int j=0; j <6 ; j++) {
 digitalWrite(pin_pump, HIGH); 
 delay (500+j*300);
 digitalWrite(pin_pump, LOW); 
 delay (1500-j*200);
}
}
myGLCD.print ("               " , CENTER ,40); 
}

void pump_zator () {
  if (st[9]==0) {
  if (st[12]==1) {
  if (digitalRead(pin_relay)==LOW) pump_work=HIGH;
  else pump_work=LOW;
  }
  else if (st[13]>0) {

  if (nagrev==true) {pump_work=HIGH;} 
  else {
  if (clock()>sump) {sump=clock()+long(st[13+npp])*1000;npp++;}
  if (npp>1 && st[14]>0) {npp=0;pump_work=LOW;}
  else if (npp==1) {pump_work=HIGH;} 
    
  int prp=(sump-clock())/1000;
  int ksrp;
  if (prp>9) ksrp=0;
  else ksrp=8;
  if (st[14]>0) {myGLCD.print (" "+String(prp)+" " , ksrp ,110); }
  }
}
  else {pump_work=LOW;}  
  
  if (t_zatora>st[10] && vpop==0) {vpop=clock()+60000;}  
  if (clock()>vpop) {
 
  if (t_zatora>=st[10]){pump_work=LOW;}
  else {vpop=0;}
 
  }
  else {pump_work=LOW;}
  
  }
  else {pump_work=HIGH;}
  digitalWrite(pin_pump, pump_work);  
}
 
void pump_varka () {
 
 
  if (st[9]==0) {
  if (st[11]==1  && st[13]>0)  {
  if (clock()>sump) {sump=clock()+long(st[13+npp])*1000;npp++;}
  if (npp>1 && st[14]>0) {npp=0;pump_work=LOW;}
  else if (npp==1) {pump_work=HIGH;}  
  int prp=(sump-clock())/1000;
  int ksrp;
  if (prp>9) ksrp=0;
  else ksrp=8;
  if (st[14]>0) {myGLCD.print (" "+String(prp)+" " , ksrp ,110);}
  }
  else {pump_work=LOW;}

  if (t_zatora>st[10] && vpop==0) {vpop=clock()+60000;}  
  if (clock()>vpop) {
 
  if (t_zatora>=st[10]){pump_work=LOW;}
  else {vpop=0;}
 
  }
  else {pump_work=LOW;}

  
  }
  else {pump_work=HIGH;}

  digitalWrite(pin_pump, pump_work);
}



void Load_recepies(char* recepies) {
  File configFile = SD.open(recepies); 
  if (configFile) 
  {
    memset(recname, 0, sizeof(recname));
    memset(recpara, 0, sizeof(recpara));
    kol_receptov=0;
    char buff[21];
    byte rec = 0, par = 0, c = 0;
 
    while(configFile.available())
    {
  if (kol_receptov>29) {configFile.close(); break;}
   
      char symb = configFile.read();
      if(symb >= 32)
      {
        if(symb == ',' || symb == ';')
        {
          
          buff[c] = '\0';
          c = 0;
          if(par == 0) strcpy(recname[rec], buff);
          else recpara[rec][par] = atoi(buff);
          par++;

          if(symb == ';')
          {
            par = 0;
            rec++;
            kol_receptov++;

          }
        }
        else { buff[c++] = symb;}
      }

  }

 configFile.close(); 
 }

}



//функция чтения данных, пришедших от web-сервера





String floattostring (float a, int b){
int ia=int(a);
int pp=int((a-ia)*pow(10,b));
return String(ia)+"."+String(pp);
}


void senddatawifi (){
String paramwifi="";  
if (beepwork==1) beeptime++;
if (beeptime>200) {beepwork=0; beeptime=0;}
String wifisendfile="";

for (int i = 0; i < kolfiles; i++) { wifisendfile+="{"+String(recfile[i])+"}";}
paramwifi=String(nstad)+"["+floattostring(t_zatora, 1)+"]["+nasoswork()+"]"+wifisendfile;
if (nstad==1) paramwifi=String(nstad)+"["+floattostring(t_zatora, 1)+"]<"+String(nomer_pauzi)+"><"+String(tp[nomer_pauzi])+">["+nasoswork()+"]["+String(beepwork)+"]["+String(dkpm)+"]";
else if (nstad==2) paramwifi=String(nstad)+"["+floattostring(t_zatora, 1)+"]["+nasoswork()+"]["+String(beepwork)+"]["+String(dkpm)+"]";
if (nstad>0 && nagrev==false && dkpm<=maxvp) paramwifi=paramwifi+"["+String(dkpm)+"]$";
else paramwifi=paramwifi+"$";
Serial3.print (paramwifi);
Serial3.flush();
}

String nasoswork() {
if (pump_work) return "1";
else return "0";
}

void SSColor (){
String ub="/"+floattostring(t_zatora,1)+"/";
myGLCD.setFont(SevenSegNumFont);
if (digitalRead(pin_relay)==HIGH) {
myGLCD.setColor(VGA_RED);
myGLCD.print (ub,CENTER,65);
myGLCD.setColor(255,255,255);
}
else myGLCD.print (ub, CENTER,65);

myGLCD.setFont(BigFont);
}

String nullvp(int a) {
if (a<10) {return "0"+String(a);}
return String(a);
}

void serialRead() {
  readingSerial = true;
  count = 0;
  
  iniReading:
  if (Serial3.available() > 0) {
    unsigned int readChar = Serial3.read();
    if (readChar == END_CHAR) {
      goto endReading;
    } else {
      serialMessage[count++] = readChar;
      goto iniReading;
    }
  }
  goto iniReading;
  endReading:
  readingSerial = false;
  WiFiReadData();
}

void WiFiReadData()
{
char ndr[21];
int  pdr[19];
char fdr[13];
char buff[21];
byte c=0 , par=0;
String WiFistr="";
boolean zvf=false;

for (int i = 0; i < count; i++) {
char rc=serialMessage[i];
          if(rc == ',' || rc == ';' || rc == '|') {
          buff[c] = '\0';
          c = 0;
          if (rc == '|') {zvf=true; strcpy(fdr, buff);}
          else {
          if (rc == ';') WiFistr+=String(buff)+";\r\n";
          else WiFistr+=String(buff)+",";
          if(par == 0) strcpy(ndr, buff);
          else pdr[par] = atoi(buff);
          par++;
          }}
          else buff[c++]=rc;
}
 WiFistr.replace("%20", " ");
  if (!zvf)  { // пишем в память
  kol_pauz=pdr[1];
  for (int j=2; j <= kol_pauz+1 ; j++) {
  tp[j-1]= pdr[j];
  vp[j-1]= pdr[j+kol_pauz];
  }
  vr_varki=pdr[kol_pauz*2+2];   
  kol_hop=pdr[kol_pauz*2+3];
  for (int j=0; j < kol_hop; j++) {  vh[j] = pdr[kol_pauz*2+4+j]; }
  }  
else if (SDin && clock()>pauza[3])  {//пишем на SD
if (SD.exists(fdr)) {
if(String(ndr)=="delete") {SD.remove(fdr);scanSD();}
else{
int rec = 1;
opfile=SD.open(fdr, FILE_READ);
while (opfile.available()) {
char symb = opfile.read();
if(symb == ';') rec++;
WiFistr+=symb ;
if (rec>29) break; 
}
opfile.close(); SD.remove(fdr);

opfile=SD.open(fdr, FILE_WRITE);
opfile.print(WiFistr); 
opfile.close();
}
}

else{
opfile=SD.open(fdr, FILE_WRITE);
opfile.println(WiFistr); 
opfile.close(); 
scanSD();
}
pauza[3]=clock()+10000;
} 
}

void scanSD() {
  kolfiles=0; nfile=0;
  memset(recfile, 0, sizeof(recfile));
  File dir=SD.open("/");
  dir.rewindDirectory();
  while(true) {
     File entry =  dir.openNextFile();
     if (!entry) {break;}
     if (!entry.isDirectory() && entry.size()>15 && entry.size()<2600 && String(entry.name()).length()<=12) {

     strcpy(recfile[kolfiles], entry.name());
     kolfiles++;
     if (kolfiles>29) { entry.close();break;}
     } 
     entry.close();
   }

}

void pumpind() {
if  ( pump_work==HIGH) myGLCD.setColor(VGA_GREEN);
else myGLCD.setColor(bgcolor[0],bgcolor[1],bgcolor[2]);
myGLCD.fillCircle (30,90,15);
myGLCD.setColor(255,255,255);
myGLCD.drawCircle (30,90,16);
}

unsigned long clock() {
if  (rtctf) {
  
if   (millis()<(dkvrtc+1000)){return ((now()-starttime)*1000+millis()-dkvrtc);}
else {dkvrtc=millis(); return ((now()-starttime)*1000);}

}
else return millis();
}



void logger() {
int aa=0;
if (menu==7){ 

write_EEPROM(20,1); 
write_EEPROM(21,nomer_pauzi);
write_EEPROM(22,kol_pauz);
byte bb=(solod) ? 1 : 0;
write_EEPROM(23,bb);

for (int j=nomer_pauzi; j <=kol_pauz; j++) {
aa=j-nomer_pauzi; 
write_EEPROM(24+aa,tp[j]); 
}
for (int j=nomer_pauzi; j <=kol_pauz; j++) {
aa=aa+1;
if (j==nomer_pauzi && !nagrev) write_EEPROM(24+aa,dkpm);
else write_EEPROM(24+aa,vp[j]);
}
}


else if (menu==14){
write_EEPROM(20,2); 
if (!nagrev) write_EEPROM(21,dkpm); 
else write_EEPROM(21,vr_varki);

int pv=vr_varki-dkpm;
for (int j=0; j <kol_hop; j++) {
if ( clock()<vnv+vh[j]*60000 ) {aa=aa+1; write_EEPROM(22+aa,vh[j]-pv); }
}
write_EEPROM(22,aa);
}

}

void write_EEPROM (int Addr,int Val) {
  delay (50);
  EEPROM.write(Addr,highByte(Val));
  delay (50);
  EEPROM.write((Addr+100),lowByte(Val));
}

int read_EEPROM (int Addr) {
return word(EEPROM.read(Addr),EEPROM.read(Addr+100));
}

void srv_hop(int j) {
//j=j+3;  
//servo.attach(j);  
//servo.write(90);
//beeper();
//servo.write(0);
//beeper();
}
