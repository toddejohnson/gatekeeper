// 
// Copyright (c) 2012, Todd E Johnson All Rights reserved.
// see LICENSE
// 
#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
IPAddress ip(192,168,1,253);
IPAddress myDns(192, 168,1,254);
IPAddress gateway(192,168,1,254);

EthernetServer server(80);
EthernetClient client;

/////////////////////
// Setup the stuff 
/////////////////////
void setup() {
  Serial.begin(9600);
  
  pinMode(2, OUTPUT);
  while (!Serial) {
    ;
  }
  Ethernet.begin(mac, ip, myDns, gateway);
  server.begin();
  
  delay(1000);
}

/////////////////////
// Main loop
/////////////////////
void loop() {
 readTag();
 checkForClient();
}

/////////////////////
// Set all chars in the array to null
/////////////////////
void clearTag(char one[]){
  for(int i = 0; i < strlen(one); i++){
    one[i] = 0;
  }
}

/////////////////////
// Open the garage door
/////////////////////
void openDoor() {
  Serial.println("open the door");
  digitalWrite(2, HIGH);
  delay(2500);
  digitalWrite(2, LOW);
}

/////////////////////
// Check if client is waiting
/////////////////////
void checkForClient() {
  EthernetClient sclient = server.available();
  String buffer;
   
  if(sclient) {
    while(sclient.connected()) {
      if(sclient.available()) {
        char c = sclient.read();
        buffer += c;
        Serial.write(c);
        
        if(c == '\n'){
          if(buffer.indexOf("GET / ")>=0){
            sclient.println("HTTP/1.1 200 OK");
            sclient.println("Content-Type: text/html");
            sclient.println("Connection: close");
            sclient.println();
            sclient.println("<!DOCTYPE HTML>");
            sclient.println("<html>");
            sclient.println("Welcome!<br>");
            sclient.println("<a href=\"/opendoor\">Open the door</a><br>");
            sclient.println("</html>");
          }else if(buffer.indexOf("GET /opendoor ")>=0){
            sclient.println("HTTP/1.1 200 OK");
            sclient.println("Content-Type: text/html");
            sclient.println("Connection: close");
            sclient.println();
            sclient.println("<!DOCTYPE HTML>");
            sclient.println("<html>");
            sclient.println("Opening...!");
            sclient.println("</html>");
            sclient.println();
            sclient.println();
            openDoor();
          }else{
            sclient.println("HTTP/1.1 404 OK");
            sclient.println("Content-Type: text/html");
            sclient.println("Connection: close");
            sclient.println();
            sclient.println("<!DOCTYPE HTML>");
            sclient.println("<html>");
            sclient.println("Not Found!");
            sclient.println("</html>");
          }
          break;
        }
      }
    }
    buffer = "";
    delay(1);
    sclient.stop();
  }
}
/////////////////////
// Check the tag
/////////////////////
void checkTag(char tag[]){
  if(strlen(tag) != 12) return; //empty, no need to contunue
  Serial.println("Checking...");
  if(connectAndRead(tag)) {
    openDoor();
  }
}

/////////////////////
// Connect to the webserver and verify
/////////////////////
boolean connectAndRead(char tag[]) {
  if(client.connect("www.my-server.com", 80)) {
    Serial.println("connecting...");
    client.print("GET /keymaster/keymaster.php?name=MYID&passcode=MYCODE&str=");
    client.print(tag);
    client.println(" HTTP/1.1");
    client.println("Host: www.my-server.com");
    client.println("User-Agent: arduino-ethernet");
    client.println("Connection: close");
    client.println();
    
    return readPage();
  }
  Serial.println("connection failed");
  client.stop();
  return false;
}

/////////////////////
// Read the page
/////////////////////
boolean readPage() {
  char inString[32];
  int index = 0;
  boolean reading = false;

  while(true) {
    if(client.available()) {
      char c = client.read();
      
      if(c == '<') {
        reading = true;
      }else if(c == '>') {
        reading = false;
        client.stop();
        client.flush();
        Serial.print("Got: ");
        Serial.println(inString);
        Serial.println("disconnect");
        if(inString[0]=='1'){
          return true;
        }else{
          return false;
        }
      }else if(reading) {
        inString[index] = c;
        index++;
      }
    }else if(!client.connected()){
      Serial.println("disconnecting");
      client.stop();
      return false;
    }
  }
}

/////////////////////
// Read the tag 
/////////////////////
void readTag() {
  char tagString [13];
  int index = 0;
  boolean reading = false;

  while(Serial.available()){
    int readByte = Serial.read(); //read next available byte
     
    if(readByte == 2) {
      reading = true; //begining of tag
      //Serial.println("Start");
      delay(150);
    }else if(readByte == 3) {
      reading = false; //end of tag
      tagString[12] = 0;
      //Serial.println("End");
    }else if(readByte == 10 || readByte == 13){
      continue;
    }else if(reading) {
      //store the tag
      //Serial.print("R:");
      //Serial.print(index);
      //Serial.print(":");
      //Serial.println(readByte);
      tagString[index] = readByte;
      index ++;
    }
  }
  
  if(strlen(tagString) == 0) return;
  Serial.println(tagString);
  checkTag(tagString);
  clearTag(tagString);
}
