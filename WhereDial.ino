/*
 * WhereDial v1.0
 * 
 * 
 */

#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <util.h>
#include <SPI.h>
#include <Ethernet.h>
#include <HttpClient.h>
#include <Stepper.h>

#include "WhereDial_Defines.h"
#include "WhereDial_Local.h"

const int stepsPerRevolution = 2048;
int motorPins[] = {A2,A5,A3,A4};
Stepper myStepper(stepsPerRevolution, motorPins[0],motorPins[1],motorPins[2],motorPins[3]);            

byte mac[] = WHEREDIAL_LOCAL_MAC;
char *userAgent = "WhereDial/1.0";

int ledRed=2;

int errorLeds[] = {7,6,5,4,3};
/*
#LEDS    ERROR
[1][0][1][0][1] => SetupEthernet problem
5        dns
4        http.get error
3        connection to the server | error code
2        parsing
1        is turning
0        no error

*/




int actual;
int prev=0;
int placeChanged=0;
int spinToggle=1;
// MAC address for your Ethernet shield
int lastPosition;
char buffer [10];  // room for 9 characters and the terminating null
EthernetClient client;
HttpClient http(client);
char lastPlaceHash[41];
char apiHostname[100];
int apiPort = 0;
char apiPath[100];
int apiRefreshTime;

IPAddress ip;



void setup()
{
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  Serial.println("Starting WhereDial...");
  myStepper.setSpeed(9);
  
  int i = 0;
  for(i=0;i<5;i++)
  {
    pinMode(errorLeds[i],OUTPUT);
    delay(50);
    digitalWrite(errorLeds[i],HIGH);
  }
  
  lastPlaceHash[0]='\0';
  
  /*
  int angle=map(5, 0, 360, 0, 12234);
  myStepper.step(-angle);
  delay(500);
  myStepper.step(angle);
  */
  setupEthernet();
  for(i=0;i<5;i++)
  {
    delay(50);
    digitalWrite(errorLeds[i],HIGH);
  }
  digitalWrite(ledRed,LOW);

  while (!getConfig()) {
    delay( 20000 );
    for(i=0;i<5;i++)
    {
      delay(50);
      digitalWrite(errorLeds[i],HIGH);
    }
    digitalWrite(ledRed,LOW);
    delay(1000);
  }
}


void loop()
{
  digitalWrite(ledRed,LOW);

  for(int i=0;i<5;i++)
  {
    delay(50);
    digitalWrite(errorLeds[i],HIGH);
  }
  Serial.println();
  delay(1000);
  if(getPage() == 1)
    turn();
  delay(apiRefreshTime*1000);
}

void setupEthernet()
{
  // Set up the Ethernet using DHCP to automatically configure an IP address
  // and the DNS server to use
  Serial.println("Setting up Ethernet");
  while (Ethernet.begin(mac) != 1)
  {
    Serial.println("Error getting IP address via DHCP, trying again...");
    digitalWrite(ledRed,HIGH);
      
    digitalWrite(errorLeds[1],LOW);
    digitalWrite(errorLeds[3],LOW);
    delay(5000);
  }
  
  digitalWrite(ledRed,LOW);

}

int getConfig()
{
  int ret=0;
  DNSClient dns;
 
  Serial.println("Retrieving config...");
  
  dns.begin(Ethernet.dnsServerIP());
  // Not ideal that we discard any errors here, but not a lot we can do in the ctor
  // and we'll get a connect error later anyway
  Serial.println("IP:");
  int dnsErr = dns.getHostByName(MAPME_AT_HOSTNAME,ip);
  if(dnsErr != 1)
  {
      Serial.print("Error with the DNS, errocode:");
      Serial.println(dnsErr); 
  }else
  {
       /* OK - TURN DOWN LED  01 */ 
      digitalWrite(errorLeds[4],LOW);
  }
  Serial.println(ip);
  Serial.println("end-ip");
  int err =0;

  Serial.println("Requesting config...");  
  err = http.get(ip, MAPME_AT_HOSTNAME,MAPME_AT_PORT, MAPME_AT_CONFIG_PATH, userAgent);
  if (err == 0)
  {
    
    /* OK - TURN DOWN LED  02 */ 
    digitalWrite(errorLeds[3],LOW);
    Serial.println("Started request ok");

    err = http.responseStatusCode();
    if ((err >= 200) && (err < 300))
    {
      
      /* OK - TURN DOWN LED  03 */ 
      digitalWrite(errorLeds[2],LOW);
      
      delay(1000);
      processConfig();
      
      /* OK - TURN DOWN LED  04 */ 
      digitalWrite(errorLeds[1],LOW);
      ret = 1;
    }
    else
    {
      Serial.print("Web request failed, got status code: ");
      Serial.println(err);
      digitalWrite(ledRed,HIGH);      
    }
  } 
  else
  {
    Serial.print("Connection failed: ");
    Serial.println(err);
    digitalWrite(ledRed,HIGH);
    
  } 
  if (!ret) {
    // Turn off LED 0 so that we can distinguish this from
    // a failed position request
    digitalWrite(errorLeds[0],LOW);
  }
  http.stop();

  Serial.println("Finished retrieving config...");
  if (ret && (apiHostname == NULL || apiPath == NULL || apiPort == 0 || apiRefreshTime == 0)) {
    ret = 0;
  }
  Serial.print("ret=");
  Serial.println(ret);
  return ret;
}

void processConfig()
{
  http.skipResponseHeaders();
  int charCount = http.readBytesUntil(',', apiHostname, sizeof(apiHostname)-1);
  Serial.print("charCount=");
  Serial.println(charCount);
  apiHostname[charCount]='\0';
  Serial.print("apiHostname=");
  Serial.println(apiHostname);

//  Serial.print("read=");
//  char c =http.read();
//  Serial.println(c);

  charCount = http.readBytesUntil(',', buffer, sizeof(buffer)-1);
  Serial.print("charCount=");
  Serial.println(charCount);
  buffer[charCount]='\0';
  Serial.print("apiPort=");
  Serial.println(buffer);
  apiPort=atoi(buffer);
  
//  Serial.print("read=");
//  c =http.read();
//  Serial.println(c);
  
  charCount = http.readBytesUntil(',', apiPath, sizeof(apiPath)-1);
  apiPath[charCount] = '\0';
  Serial.print("apiPath=");
  Serial.println(apiPath);
  
//  Serial.print("read=");
//  c =http.read();
//  Serial.println(c);
  
  charCount = http.readBytesUntil(',', buffer, sizeof(buffer)-1);
  Serial.print("charCount=");
  Serial.println(charCount);
  buffer[charCount]='\0';
  Serial.print("apiRefreshTime=");
  Serial.println(buffer);
  apiRefreshTime=atoi(buffer);
}

int getPage()
{
  int ret=0;
  DNSClient dns;
 
  dns.begin(Ethernet.dnsServerIP());
  // Not ideal that we discard any errors here, but not a lot we can do in the ctor
  // and we'll get a connect error later anyway
  Serial.println("IP:");
  int dnsErr = dns.getHostByName(apiHostname,ip);
  if(dnsErr != 1)
  {
      Serial.print("Error with the DNS, errocode:");
      Serial.println(dnsErr); 
  }else
  {
       /* OK - TURN DOWN LED  01 */ 
      digitalWrite(errorLeds[4],LOW);
  }
  Serial.println(ip);
  Serial.println("end-ip");
  int err =0;

  Serial.println("Requesting new angle...");
  // We need to add &position=DDD&placeHash=40CHARS
  int apiPathSize = strlen(apiPath);
  int pathSize = apiPathSize+64;
  char path[pathSize];
  char *pathPointer = path;
  strncpy(path,apiPath,apiPathSize);
  char* queryString = strstr(apiPath,"?");
  pathPointer += apiPathSize;
  *pathPointer = '?';
  if (queryString) {
    *pathPointer = '&';
  }
  ++pathPointer;

  strcpy(pathPointer,"position=");
  pathPointer+=9;  
  char pos[4];
  sprintf(pos,"%d",actual);
  strcpy(pathPointer,pos);
  pathPointer += strlen(pos);
  
  strcpy(pathPointer,"&placeHash=");
  pathPointer+=11;
  strcpy(pathPointer,lastPlaceHash);
  pathPointer+=strlen(lastPlaceHash);
  
  Serial.print("Calculated path=");
  Serial.println(path);
  
  err = http.get(ip, apiHostname,apiPort, path, userAgent);
  if (err == 0)
  {
    
    /* OK - TURN DOWN LED  02 */ 
    digitalWrite(errorLeds[3],LOW);
    Serial.println("Started request ok");

    err = http.responseStatusCode();
    if ((err >= 200) && (err < 300))
    {
      
      /* OK - TURN DOWN LED  03 */ 
      digitalWrite(errorLeds[2],LOW);
      
      delay(100);
      processPage();
      
      /* OK - TURN DOWN LED  04 */ 
      digitalWrite(errorLeds[1],LOW);
      ret = 1;
    }
    else
    {
      Serial.print("Web request failed, got status code: ");
      Serial.println(err);
      digitalWrite(ledRed,HIGH);      
    }
  } 
  else
  {
    Serial.print("Connection failed: ");
    Serial.println(err);
    digitalWrite(ledRed,HIGH);
    
  } 
  http.stop();
  return ret;
}

void processPage()
{
  http.skipResponseHeaders();
  int charCount = http.readBytesUntil(',', buffer, sizeof(buffer)-1);
  Serial.print("charCount=");
  Serial.println(charCount);
  buffer[charCount]='\0';
  Serial.println(buffer);
  prev=actual;
  actual=atoi(buffer);
  Serial.println();
  Serial.println(prev);
  Serial.println(actual);
  
//  Serial.print("read=");
//  char c =http.read();
//  Serial.println(c);
  
  char placeHash[41];
  charCount = http.readBytesUntil(',', placeHash, sizeof(placeHash)-1);
  placeHash[charCount] = '\0';
  Serial.print("placeHash=");
  Serial.println(placeHash);
  Serial.print("lastPlaceHash=");
  Serial.println(lastPlaceHash);
  placeChanged = strcmp(placeHash,lastPlaceHash);
  strcpy(lastPlaceHash,placeHash);
}

void turn()
{
  int diff = actual-prev;
  int angle=map(diff, 0, 360, 0, 12234);
  myStepper.step(-angle);
  if (!angle && placeChanged) {
    myStepper.step(12234 * spinToggle);
    spinToggle = 0 - spinToggle;
  }
  /* OK - TURN DOWN LED  05 */ 
  digitalWrite(errorLeds[0],LOW);
  
  /* TURN OFF THE MOTOR PINS, SAVE POWER & HEAT */
  for (int i = 0; i < 4; ++i) {
    digitalWrite(motorPins[i],LOW);
  }
}

