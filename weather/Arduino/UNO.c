#include <Wire.h> 
#include <dht11.h>                                                                 
#define DHT11PIN 2 
    
#define SLAVE_ADDRESS 0x04  

int number = 0;  
int state = 0;  
int temperature = 0;
int humidity = 0;
char retMSG[16] = "";  
  
dht11 DHT11;     
   
void setup() {  
   Serial.begin(9600);                       
   Serial.println("DHT11 TEST PROGRAM ");       
   Serial.print("LIBRARY VERSION: ");          
   Serial.println(DHT11LIB_VERSION);             
   Serial.println();    

   Wire.begin(SLAVE_ADDRESS);  
     
   Wire.onReceive(receiveData);   
   Wire.onRequest(sendData);   
}
    
void loop() {  
   delay(100);    

   Serial.println("\n");                            
   int chk = DHT11.read(DHT11PIN);               

   // for ARDUINO OUPUT 
   Serial.print("Humidity (%): ");                     
   Serial.println((float)DHT11.humidity, 2);    
   Serial.print("Temperature (oC): ");             
   Serial.println((float)DHT11.temperature, 2);  

   temperature = (int)DHT11.temperature;
   humidity = (int)DHT11.humidity;
   sprintf(retMSG, "%2d'C %2d%%\n", temperature, humidity);
   
   delay(2000);
}  
  
void receiveData(int byteCount){  
   
 while(Wire.available()) {  
    Serial.println(retMSG);
    break;
   }  
}  
  
void sendData(){  
    Wire.write( retMSG, strlen(retMSG) );  
}  
 
 
