/**
*
* title: arduino coffee sensor firmware
* author: jared smith
* date: February - March, 2013
*
**/
#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>


// vars for MAC
byte mac[6] = { 0xBA, 0xBE, 0x00, 0x00, 0x00, 0x00 };
char macstr[18];

// vars for averaging the sensor's reading
int minLight;
int maxLight;
int lightLevel;
int averageLight;

// map EthernetClient to client
EthernetClient client;

// enter your server's IP here as a coma-sep string
IPAddress server(50,56,34,229);

// the analog pin to read values from
int pin = 0; 

// the threshold to assume that a cup has been set down
int darkLevel = 5;

// the threshold to assume that a cup has been picked up
int brightLevel = 40;

// the threshold for the count to get to before triggering a cup
int timeToCup = 40;

// placeholder for a counter object after the light has gone low
int count = 0;

// bool to return true if light is low
boolean triggered = false;

// placeholder for a timer object to allow us to run sequences at specific intervals
int timer = 0;

void setup(){
  // begin serial
  Serial.begin(9600);
  
  // grab a base reading from our analog sensor 
  // so that reCheckLight() has something to start from
  lightLevel=analogRead(pin);
  minLight=lightLevel-20;
  maxLight=lightLevel;
  
  // auto-generate MAC adress and then store it in eeprom 
  // this will keep the value even when the arduino is powered off
  if (EEPROM.read(1) == '#'){
    for (int i = 2; i < 6; i++){
      mac[i] = EEPROM.read(i);
    }
  }else{
  randomSeed(analogRead(0));
    for (int i = 2; i < 6; i++){
      mac[i] = random(0, 255);
      EEPROM.write(i, mac[i]);
    }
    EEPROM.write(1, '#');
  }
  snprintf(macstr, 18, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  while(!Serial){
    ;
  }
  
  // print our values so that we know we have network
  // connectivity working correctly
  Serial.println("MAC: ");
  Serial.print(macstr);
  Ethernet.begin(mac);
  Serial.println("IP: ");
  Serial.println(Ethernet.localIP());
}

// this function simply runs at intervals to return a new average
// light level for that time period, that way we don't get mis-triggers
// if the light changes in the room (ie. someone closes the blinds)
void reCheckLight(){
  // we assume if reCheckLight() is running that a cup is not there
  triggered = false;
  
  // get new reading from sensor
  lightLevel=analogRead(pin);
  minLight=lightLevel-20;
  maxLight=lightLevel;
  
  // let us know that it ran  
  Serial.println("checkinglight");
}

void loop(){
  // start the timer 
  timer++;
  
  // grab a new base reading from our analog sensor
  lightLevel=analogRead(pin);
   
  // average out and map our sensor value to a useable range
  if(minLight>lightLevel){
    minLight=lightLevel;
  }
  if(maxLight<lightLevel){
    maxLight=lightLevel;
  }
  
  //Adjust the light level to produce a result between 0 and 100.
  averageLight = map(lightLevel, minLight, maxLight, 0, 100); 
   
   // let us know what the average is for this period
  Serial.println(averageLight);
  
  // every 10 times through the loop this will trigger reCheckLight()
  // to understand why, see the reCheckLight function
  if((timer % 10) == 0 && triggered != true){
    reCheckLight();
  }
  // delay it a bit - no need to run this too rapidly
  delay(250);
  
  // trigger the cycle() function
  cycle();
}

// this function is triggered every time through the main loop
// its purpose is to determine if the light has dropped below 
// our darkLevel threshold and if so, proceed to increase the
// count var and then, if we've remained in this loop for longer
// than our timeToCup threshold then we go ahead and sendData()
void cycle(){
  if(averageLight <= darkLevel){
    triggered = true;
    count++;
    if(count == timeToCup){
      Serial.println("hello coffee cup");
      sendData();
    }
    Serial.println(count);
  }
  // if the level jumps back up above our brightLevel threshold
  // then we break out of the cycle() and reset the whole board
  // this is useful too as it reestablishes the counter and timer
  // vars so that the arduino does not have a chance to overload itself
  if(averageLight >= brightLevel){
    count = 0;
    if(triggered == true){
      Serial.println("reset");
      reset();
    }
  }   
}

// send data to the server
void sendData(){
  if(Ethernet.begin(mac) == 0){
    for(;;)
      ;
  }
  delay(1000);
  // adjust your port number as needed
  if(client.connect(server, 80)){
    // enter your header information to send to the server here
    client.println("GET /coffee/add HTTP/1.1");
    client.println("Host: coffeetracker.jvst.us");
    Serial.println(client.status());
    client.println("Connection: close");
    client.println();
  }else {
    Serial.println("connection failed");
    Serial.println("disconnecting.");
    client.stop();
    reset();
  }
  client.stop();
  while(client.status() != 0){
    delay(5000);
  }
}

void reset(){
  asm volatile ("  jmp 0"); 
};
