#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7,3,POSITIVE);  // Set the LCD I2C address

int VoltDrop;
const word    TIRE2=160; //tire perimeter /100 or centimeters
const float   ILI=952.38; //gasoline sensor>> Liters per impulse X10000000
const long    TotalTank7=105000000;       //10.5 Liters


volatile byte  speed;    //interrupt var-Perimetro~=1.6m  140km/h>>24Hz    30km/h>>5Hz
volatile byte  gasoline;  //interrupt var  10500impulses liter H2O at 20ºC  6L/100kmh>>17Hz
 
 //the number at the end of vars is the decimal places to be assumed, to avoid using float's
 
 
 //This vars are stored in EEprom   vvvvv
 long          Ld7;        //liters on tank /10000000
 long          Kt5;        //total kilometers since reset /100000
 byte          VM;         //max speed registered
 long          Lt7;        //liters spent since reset  /10000000
 //This vars are stored in EEprom   AAAAA

 
 long          Ktemp5=0;        //total kilometers /100000
 unsigned long time=0;       //time spent from milis()
 unsigned long timeOld1=0;   //tine old velocity block
 unsigned long timeOld2=0;   //time old gasoline comsuption block
 unsigned long gaS;           //temp var 
 byte          Vi=0;        //Instantaneous(last 1 second) velocity
 word          Li2=0;       //Instantaneous(last 3 seconds) gas comsuption /100
 word          Lm2=0;       //average gas comsuption since reset /100
 
 int buttonState1 = 0;         // variable for reading the pushbutton VIEW status
 int buttonState2 = 0;         // variable for reading the pushbutton RESET status
 int buttonState3 = 0;         // variable for reading the pushbutton TANK RESET status
 
 int buttonState1_old = 0;         // variable for reading the pushbutton VIEW old
 int buttonState2_old = 0;         // variable for reading the pushbutton RESET old
 int buttonState3_old = 0;         // variable for reading the pushbutton TANK old
 
 const int buttonPin1 = 9;      // the number of the pushbutton pin VIEW
 const int buttonPin2 = 10;     // the number of the pushbutton pin RESET
 const int buttonPin3 = 11;     // the number of the pushbutton pin TANK RESET
 
 int ViewFlag=0;
 
 void setup()
 {
   Serial.begin(115200);
   attachInterrupt(0, gasoline_detect, RISING);//inialize   the intterrupt pin (Arduino digital pin 2)
   attachInterrupt(1, speed_detect, RISING);//Initialize the intterrupt pin (Arduino digital pin 3)
   speed = 0;
   gasoline = 0;
   
   //ler var's da eeprom

	
int eeAddress = 0;          //Location we want the data to be put

EEPROM.get(eeAddress, Ld7);   //One simple call, with the address first and the object second.
eeAddress += sizeof(long); //Move address to the next byte after the data type size.

EEPROM.get(eeAddress, Kt5);   //One simple call, with the address first and the object second.
eeAddress += sizeof(long); //Move address to the next byte after the data type size.

EEPROM.get(eeAddress, VM);   //One simple call, with the address first and the object second.
eeAddress += sizeof(byte); //Move address to the next byte after the data type size.

EEPROM.get(eeAddress, Lt7);   //One simple call, with the address first and the object second.


Serial.println("ALL RETRIEVED!");


   
   pinMode(buttonPin1, INPUT); ////  1 view          TO DO escolher e testar na bredboard pinos
   pinMode(buttonPin2, INPUT);////   2 reset         TO DO escolher e testar na bredboard pinos
   pinMode(buttonPin3, INPUT);////   3 tank reset    TO DO escolher e testar na bredboard pinos
   
   lcd.begin(16,2);               // initialize the lcd 
 }



 void loop()
 {
   ////velocity block DONE!!
   time= millis();
   if (time > (timeOld1+1000))   // read speed at 1 second interval
   { 
	 Kt5+=speed*TIRE2;           //add number of rotations x perimeter to Total Km both vars in cm
	 
	 //calculate speed taking note of the decimal places 
	 //36 refers to 3600 seconds in a hour
	 //as Vi is byte the calculation drops decimal places
	 Vi=(speed*TIRE2*36)/time;	
	 
	 if (Vi>VM)   //check if current speed is a new record speed
	 {
	  VM=Vi;      //save top speed
	  }   
  //////BLOCO LCD//  DONE!!
	
  lcd.clear(); // cls
  
  //// Giving the proper decimal places to vars converting to strings trimming decimal places //
  String sVi= String (Vi);
  
  float  fLi2=Li2/100;
  String sLi2= String(fLi2, 2);
  
  float  fLm2=Lm2/100;
  String sLm2= String(fLm2, 2);
  
  float  fLd7=Ld7/10000000;
  String sLd7= String(fLd7, 2);
  
  float  fKt5=Kt5/100000;
  String sKt5= String(fKt5, 2);
  
  String sVM= String(VM);
  //upper line
  lcd.setCursor ( 0, 0 );
  lcd.print("Vi ");
  lcd.print(sVi); 
  
  lcd.setCursor ( 10, 0 );     
  lcd.print ("Li ");     
  lcd.print (sLi2);
  //lower line depends on VIEW flag
  if (ViewFlag==0) 
  {
  lcd.setCursor ( 0, 1 );
  lcd.print("Lm ");
  lcd.print(sLm2); 
  
  lcd.setCursor ( 9, 1 );     
  lcd.print ("Ld ");     
  lcd.print (sLd7);  
	  
  }
  else
  {
  lcd.setCursor ( 0, 1 );
  lcd.print("VM ");
  lcd.print(sVM); 
  
  lcd.setCursor ( 7, 1 );     
  lcd.print ("Kt ");     
  lcd.print (sKt5);  
  }
  ////////LCD BLOck end
  
   speed=0;               //zero interrupt var for next count
   timeOld1=time;         //var to count time to check time passing
   }
   
   ////gasoline spent block DONE!!
   time= millis();
   if (time > (timeOld2+3000))    // read gasoline at 3 second interval
   {
	   
	 gaS=gasoline*ILI;	//gasoline last 3 secondsXliters/impulse CONST ,decimal places corrected								
	 Lt7+=gaS;          //add to liters spent since reset
	 Ld7-=gaS;          //subtract from gas tank
	 Li2=0;           //
	 if (Vi>30)          //only calculate Li if speed > 30 if not the if is not executed and Li=0
	 {
	 Li2=100*gaS/(Kt5-Ktemp5);   //calculate   L/100km instataneous
	 }
	 
	 	 
   gasoline=0;        //zero interrupt var for next count
   timeOld2=time;     //var to count time to check time passing
   Ktemp5=Kt5;     // var to count km till next 3sec
   }
//////Buttons FLAGS Block DONE!!!
// 1 VIEW //

buttonState1 = digitalRead(buttonPin1);
  // check if the VIEW  pushbutton altered state to avoid repetition 
if (buttonState1 != buttonState1_old) //if state change
{
if 	(buttonState1==HIGH)   //and is HIGH
	{
		ViewFlag= !ViewFlag;//change the flag View
				
	}
	
	buttonState1_old=buttonState1; //store for next round
}


// 2 RESET //

buttonState2 = digitalRead(buttonPin2);
  // check if the VIEW  pushbutton altered state to avoid repetition 
if (buttonState2 != buttonState2_old) //if state change
{
if 	(buttonState2==HIGH)   //and is HIGH
	{
	  Kt5=0; //Reset KM done
	  Lt7=0; //Reset  liters spent
	  VM=0;  //Reset Top Speed
	}
	
	buttonState2_old=buttonState2; //store for next round
}



// 3 TANK RESET //

buttonState3 = digitalRead(buttonPin3);
  // check if the VIEW  pushbutton altered state to avoid repetition 
if (buttonState3 != buttonState3_old) //if state change
{
if 	(buttonState3==HIGH)   //and is HIGH
	{
		Ld7=TotalTank7;
				
	}
	
	buttonState3_old=buttonState3; //store for next round
}


/////////Write EEprom Block

VoltDrop = analogRead(A0);
Serial.println(VoltDrop);

if (VoltDrop<200)
{
	
int eeAddress = 0;          //Location we want the data to be put

EEPROM.put(eeAddress, Ld7);   //One simple call, with the address first and the object second.
eeAddress += sizeof(long); //Move address to the next byte after the data type size.

EEPROM.put(eeAddress, Kt5);   //One simple call, with the address first and the object second.
eeAddress += sizeof(long); //Move address to the next byte after the data type size.

EEPROM.put(eeAddress, VM);   //One simple call, with the address first and the object second.
eeAddress += sizeof(byte); //Move address to the next byte after the data type size.

EEPROM.put(eeAddress, Lt7);   //One simple call, with the address first and the object second.



Serial.println("ALL SAVED!");
}



 }

// INTERRUPTS BLOCK//

 void speed_detect()//This function is called whenever a magnet/interrupt from speed sensor is detected by the arduino
 {
   speed++;
   Serial.println("speed");
 }
 
 void gasoline_detect()//This function is called whenever a magnet/interrupt from gasoline flow sensor is detected by the arduino
 {
   gasoline++;
   Serial.println("gasoline");
 }
