#include <avr/pgmspace.h>
#include <Bounce2.h>
#include <Wire.h>
#include <LiquidCrystal.h>
//Liad - Start
#include <EEPROM.h>
#include "EEPROMAnything.h"
//Liad - End

LiquidCrystal lcd(14, 15, 16, 17, 18, 19);
#define encoder0PinA  4 //encoder
#define encoder0PinB  2 //encoder
Bounce debouncer = Bounce(); 
Bounce debouncer1 = Bounce(); 
#define DATA	5		/* Serial Data */
#define CLK	6		/* Serial Clock */
#define LE	7		/* Synthesizer Latch Enable */
unsigned long int FMem[]={1450001,1450001,1450001,1450001,1450001,1450001,1450001,1457003,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001,1450001};
unsigned long int Frq=145000;
unsigned long time1;
int EncDo=1;// this changes when you turn the encoder +1 or -1
long Step=5;// vfo step size
int Menue=0; // changes when you scroll the menue
boolean Mode=1; // 0= vfo 1=memory
int Mem=-1;// first memory when turn on
boolean Curspress=0;// knows when the encoder was pressed
int Shift=0;// repeater shift register 0= simp// <0 -// >0 +
int CursPos=0;// rotary encoder change
int Tmp=0;// nice to have
boolean PlSq=0;// ctcss or cor 0 ctcss 1 cor
boolean Tx=0;// when the rig is in tx it is 0
boolean TxOld=0;// can do without it read insted of reg leave for now....
boolean Rx=0;//indicates the Rx on
boolean Power=0; //output power

unsigned long int NewMemToSave = 0;
boolean SaveVfo = 0;
boolean Scan=0; // temp test scan function
int MemBeforeScan = 0;// return to this frequency when stoping the scan without ptt

void setup()
{
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
 debouncer.attach(encoder0PinA);
 debouncer.interval(2);
 debouncer1.attach(3);
 debouncer1.interval(5);
 lcd.begin (8,2);
 lcd.print("MaxDroid");
 delay(1000);
 lcd.clear();
 pinMode (9,OUTPUT);// plsql pin 13 j8 on maxtrac
 pinMode (11,OUTPUT);// power out
 pinMode(encoder0PinA, INPUT_PULLUP); // for encoder turn
 pinMode(encoder0PinB, INPUT_PULLUP); // for encoder turn 2
 pinMode(3, INPUT_PULLUP); // for encoder  press
 pinMode(8, INPUT); //pttfrom pin 11 j8 on maxtrac
 pinMode(10, INPUT); //Rx indicator from maxtrac
 pinMode (LE,     OUTPUT);
 drive_bus (0);
  digitalWrite (LE, LOW);
  digitalWrite (CLK, LOW);
  TIMSK1 = 0;			/* Disable all timer1 interrupts */
  TCCR1A = 0;			/* Normal: Don't drive OC1A/OC1B */
  TCCR1B = _BV(WGM12) | _BV(CS10); /* CTC Mode 4, clk/1 no prescale */
  OCR1A = 888;			/* 889 ticks between interrupts */
  
 // if(EEPROM[0] == 255 && EEPROM[1] == 255 && EEPROM[2] == 255 && EEPROM[3] == 255)
 // {
    EEPROM_writeAnything(0, FMem);
 // }
 // else
 // {
 //   EEPROM_readAnything(0, FMem);
 // }
}

void set_pll (unsigned long int hz)
{
 unsigned int n;	  /* N is the 10-bit divide-by-127 counter  */
 byte a;		  /* A is the 7-bit remainder counter */
hz*=1000;
  hz += (45100000*Tx)-(!Tx*Shift*-600000);		/* first mixer: 45.1 MHz */
  hz /= 5000;	    // Divide by 5KHz reference
  n = hz / 64;  // 127 for UHF
  a = hz - (n * 64);  // 127 for UHF
  drive_bus (1);

  /* Send "(n << 8)|(a << 1)", 24 bits, MSB first, LSB always zero */
  emit_byte ((n >> 8) & 0xFF);	/* N high byte */
  emit_byte (n & 0xFF);		/* N low byte */
  emit_byte (a << 1);		/* A and LSB 0 */
  pulse_le();			/* Latch it */

 
  emit_byte (0x16);		// 0x0901 is R=1152<<1 with LSB flag set but i wanted 5K step so change new settings
  emit_byte (0x81);
  pulse_le();

  drive_bus (0);
}
volatile unsigned long int time = 0;
void drive_bus (byte enable)
{
#if TRACES_CUT
enable = 1;	
#endif

  if (enable) {
    pinMode (DATA, OUTPUT);
    pinMode (CLK, OUTPUT);
    digitalWrite (DATA, HIGH);
  } else {
    pinMode (DATA, INPUT);
    pinMode (CLK, INPUT);
    digitalWrite (DATA, LOW);
  }
}


void emit_byte (byte c)
{
  byte bit;

  for (bit = 0; bit < 8; bit++)
    {
      digitalWrite (DATA, (c & 0x80) ? HIGH : LOW);
      time++;
      time++; 
      digitalWrite (CLK, HIGH);	/* rising edge latches data */
      c <<= 1;			
      time++;
      time++;
      time++;
      digitalWrite (CLK, LOW);
    }


  digitalWrite (DATA, HIGH);
}
void pulse_le (void)
{
  digitalWrite (LE, HIGH);
  time++;			
  time++;
  digitalWrite (LE, LOW);
}
void cursorpress()
{
  Curspress=1;
  if (Mode==1 && Menue==1)
  {
    Menue=6;
    return;
  }
  if(Mode==0 && Menue==0 && NewMemToSave != 0)
  {
    SaveVfo = 0;
    Menue = 5;
    return;
  }
  if(Menue == 5 && Mode == 0)
  {
    Menue = 0;
    Mode = 1;
    if(SaveVfo)
    {
      FMem[Mem] =  NewMemToSave;
      EEPROM_writeAnything(0, FMem);
    }
    Frq = FMem[Mem] / 10;
    SaveVfo = 0;
    NewMemToSave = 0;
    return; 
  }
  if (Menue==8)
  {
    Menue=0;
    return;
  }
  if (Menue<8)
  {
    Menue++;
    if(Menue == 5)
      Menue = 6;
    if(Menue == 7 && Mode == 0) //skip the scan menu in vfo mode
    {
       Menue=0; 
    }
  }
  else 
    Menue=0;
}
 
 void WriteFrq (unsigned long int Frq1)
 {
     lcd.setCursor ( 0, 0 );
    
     lcd.print(Frq1/1000);
          lcd.print(".");
          Tmp=Frq1%1000;
          if (Tmp<100)
          lcd.print("0");
          if (Tmp<10)
          lcd.print("0");
           lcd.print(Tmp);
           lcd.setCursor (7,0);
          if(Shift<0)
          lcd.print("-");
          if(Shift>0)
          lcd.print("+");
          if(Shift==0)
          lcd.print(" ");
          lcd.setCursor (3,1);
          if(PlSq==0)
          lcd.print(" T");
          if(PlSq==1)
          lcd.print(" C");
          digitalWrite(9,PlSq);
          lcd.setCursor ( 0, 1 );
           if (!Mode)
     lcd.print("    ");
     else
     {
       lcd.print("M");
       lcd.print(Mem);
       if(Mem<10) lcd.print(" ");
     }
     lcd.setCursor(5,1);
     if(Power) lcd.print("H");
     else lcd.print("L");
 }
 
void WriteMenue ()
{
  if (Menue==0)
  {
    lcd.clear();
    WriteFrq(Frq);
    Curspress=0;
  }
  if (Menue==1)
  {
    lcd.clear();
    lcd.setCursor ( 0, 0 );
    lcd.print("MODE  ");
    lcd.setCursor ( 0, 1 );
    if(Mode==0)
    {
      lcd.print("VFO");
    }
    else if(Mode==1)
    {
      lcd.print("Memory");
    }
      Curspress=0;
  }
    
  else if (Menue==2)
  {
    lcd.clear();
    lcd.setCursor ( 0, 0 );
    lcd.print("SHIFT  ");
    lcd.setCursor ( 0, 1 );
    if (Shift==0)
      lcd.print("Simplex");
    if (Shift==-1)
      lcd.print("Rpt -");
    if (Shift==1)
      lcd.print("Rpt +");
      Curspress=0;
  }
  else if (Menue==3)// pl squelch
  {
    lcd.clear();
    lcd.setCursor ( 0, 0 );
    lcd.print("Squelch  ");
    lcd.setCursor ( 0, 1 );
    if(PlSq==0)
      lcd.print("Pl");
    if(PlSq==1)
      lcd.print("Cor");
      Curspress=0;
  }  
  else if (Menue==4)// step
  {
    lcd.clear();
    lcd.setCursor ( 0, 0 );
    lcd.print("VFO Step  ");
    lcd.setCursor ( 0, 1 );
    lcd.print(Step);
    lcd.print("Khz");
    Curspress=0;
  }
  else if (Menue==5 && Mode == 0)// save vfo
  {
    lcd.clear();
    lcd.setCursor ( 0, 0 );
    lcd.print("Save?  ");
    lcd.setCursor ( 0, 1 );
    if(SaveVfo)
      lcd.print(" Yes  ");
    else
     lcd.print(" No  ");
    Curspress=0;
  }
  else if (Menue==6)// power
  {
    lcd.clear();
    lcd.setCursor ( 0, 0 );
    lcd.print("Power  ");
    lcd.setCursor ( 0, 1 );
    if(Power)
      lcd.print(" 30w  ");
    else
      lcd.print(" 10w  ");
      Curspress=0;
  }
  else if(Menue==7 && Mode == 1)// scan
  {
    lcd.clear();
    lcd.setCursor ( 0, 0 );
    lcd.print("Scan  ");
    lcd.setCursor ( 0, 1 );
    lcd.print("           ");
    Curspress=0; 
  }    
}
 
void addShiftToNewMem()
{
  if(Shift < 0)
    NewMemToSave = Frq * 10 + 3;
  else if(Shift > 0)
    NewMemToSave = Frq * 10 + 5;
  else
    NewMemToSave = Frq * 10 + 1;
}
 
void loop(void)
{ 
  debouncer.update();
  if ( debouncer.fell() ) 
    if (digitalRead(encoder0PinA) == digitalRead(encoder0PinB)) 
    EncDo=-1;
  else 
    EncDo=1;
   debouncer1.update();
if ( debouncer1.fell() ) 
cursorpress();  
  if(Curspress==1)
  WriteMenue ();
  Tmp=digitalRead(10);
  if(Tx)
  if(Rx!=Tmp)
{
  Rx=Tmp;
   lcd.setCursor ( 6, 1 );
  if(Rx)
 lcd.print("Rx");
 else lcd.print("  ");
}
  
 Tx=digitalRead(8);
 if(Tx!=TxOld)
 {
  set_pll(Frq);
 lcd.setCursor ( 6, 1 );
 if(Tx==0)
  {
    lcd.print("Tx");
    Menue=0;
  }
 else lcd.print("  ");
  WriteFrq(Frq+(!Tx*Shift*600));
  TxOld=Tx;
 }
  if(EncDo||0)
  {
    if (Menue==0) //VFO
    {
      if(!Mode)// VFO
      {
        if(EncDo>0) Frq=Frq+Step;
        if(EncDo<0) Frq=Frq-Step;
        EncDo=0;
        set_pll(Frq);
        WriteFrq(Frq);
        addShiftToNewMem();
        
      }
      else // memory mode
      {
        if(EncDo>0 && Mem< (sizeof (FMem)/4)-1) 
          Mem++;
        else if(EncDo>0)
          Mem=0;
        if((EncDo<0)&&(Mem>0)) 
          Mem--;
        else if(EncDo<0)
          Mem=sizeof (FMem)/4-1;
        EncDo=0; 
      ///////////////////////////////////////////////////////
       Frq=FMem[Mem]/10;
       PlSq=FMem[Mem]%2;
       Tmp=FMem[Mem]%10;
       if(Tmp>4) Shift=1;
       else if (Tmp>2) Shift=-1;
       else Shift=0;
       WriteFrq(Frq);
       set_pll(Frq);
     }
   }
  else if (Menue==1)// Mode
  {
    Mode=!Mode;
    EncDo=0;
    WriteMenue ();
    if(Mode)
    {
      Frq=FMem[Mem]/10;
      PlSq=FMem[Mem]%2;
      Tmp=FMem[Mem]%10;
      if(Tmp>4) Shift=1;
      else if (Tmp>2) Shift=-1;
      else Shift=0;
      set_pll(Frq);
    }
   }
  
  else if (Menue==2)// shift
  {
    if(EncDo>0)
    {
   if(Shift<1)
   Shift++;
   else Shift=-1;
    }
        if(EncDo<0)
    {
   if(Shift>-1)
   Shift--;
   else Shift=1;
    }
    EncDo=0;
    WriteMenue ();
    
  addShiftToNewMem();
}
  
  else if (Menue==3)// PL SQUELCH
  {
   PlSq=!PlSq;
       EncDo=0;
        digitalWrite(9,PlSq);
    WriteMenue ();
  }
  else if (Menue==4)// step
  {
    if(EncDo>0)
    {
      if (Step==2) Step=5;
      else if (Step==5) Step=10;
      else if (Step==10) Step=12;
      else if (Step==12) Step=20;
      else if (Step==20) Step=25;
      else if (Step==25) Step=50;
      else if (Step==50) Step=1000;
    }
    if(EncDo<0)
    {
       if (Step==1000) Step=50;
       else if (Step==50) Step=25;
       else if (Step==25) Step=20;
       else if (Step==20) Step=12;
       else if (Step==12) Step=10;
       else if (Step==10) Step=5;
       else if (Step==5) Step=2;
    }
    EncDo=0;
    WriteMenue ();
  }
  else if (Menue==5 && Mode == 0)// save vfo
  {
    SaveVfo =!SaveVfo;
    EncDo=0;
    WriteMenue();
  }
  else if (Menue==7 && Mode == 1)// scan
  {
    Scan=1;
    EncDo=0;
    MemBeforeScan = Mem;
    scan();
  }
  else if (Menue==6)// shift
  {
    Power=!Power;
    EncDo=0;
    digitalWrite(11,Power);
    WriteMenue ();
  }
  }
}

void scan()
{
  int Delay1=300;
  while(Scan)
  {
    if( Mem< sizeof (FMem)/4-33) ///// dont want to scan last 32 mem////
      Mem++;
    else Mem=0;
    Frq=FMem[Mem]/10;
    PlSq=FMem[Mem]%2;
    Tmp=FMem[Mem]%10;
    if(Tmp>4) Shift=1;
    else if (Tmp>2) Shift=-1;
    else Shift=0;
    WriteFrq(Frq);
    set_pll(Frq);
    time1=millis();
    Delay1=300;
    while((millis()-time1)<Delay1)
    {
      debouncer.update();
      if(digitalRead(10))
      {
        Delay1=5000;
        time1=millis();
      }
      if(!digitalRead(8))
      {
        Scan=0;
        Menue=0;
        return;
      }
      if(debouncer.fell())
      {
        Scan=0;
        Menue=0;
        Mem = MemBeforeScan;
        Frq=FMem[Mem]/10;
        PlSq=FMem[Mem]%2;
        WriteFrq(Frq);
        set_pll(Frq);
        return;
      }
    }    
  }
}


