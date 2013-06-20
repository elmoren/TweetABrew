#include <DallasTemperature.h>
#include <Ethernet.h>
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <SPI.h>
#include <Twitter.h>

/************************************************************************************************
 *
 *                          TweetABrew - Home Brew Temperature Monitor
 *
 * Author:   Nathan Elmore
 * Date:     June, 2013
 *
 * Description: This project is uses the DS18B20 temerature sensor to moniter a home brew during
 * the fermentation stage. It sends tweets at specified intervals or when they temp is out of 
 * specified range.
 *
 * Future work:
 * 1. Temperature regulation: I would like to aquire a minifridge and add hardware so one could 
 *    program the arduino to 
 * 2. Add buttons/potentiometer so that the desired temp can be set without changing the 
 *    code.
 * 3. Add another temperature sensor to moniter the temperature inside the fermentation vessel
 ***********************************************************************************************/

#define ONE_WIRE_BUS 2

/*
 * Program constants for times and allowable temperatures.
 */
const unsigned long tweetInterval = 3600000; // 1 Hour Between temp tweets
const unsigned long warnInterval  = 3600000; // 1 Hour Between Warning tweets
const unsigned long tempInterval  = 5000; // 5 seconds between temp reading

/*
 * Init LCD
 */ 
LiquidCrystal lcd(6, 7, 8, 9, 5, 3);

/*
 * Init OneWire, and DallasTemperature
 */
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

/*
 * The previous temp reading is stored because the sensor sometimes reads noise. It seems to
 * like 0°C, 85°C and -127°C
 */ 
float lastTempC;
const float minTempC = 0.0;  // Set at extremes when There is no beer brewing
const float maxTempC = 100.0;

// Token to Tweet (get it from http://arduino-tweet.appspot.com/)
Twitter twitter("1418815172-7FhPTRq9D3odnGstEOgDdEcBQzrXZxzGnF040Nv");
char tweet[155];

// Tweet containing temperature info
unsigned long lastTweet;
unsigned long lastTemp;
unsigned long lastWarn;

/**
 * Inits ethernet, uses DHCP
 */
int initEthernet() {
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  Ethernet.begin(mac);
}

/** 
 * sendTweet() : Attempts to send tweet stored in the tweet[] array
 *
 * Returns: 0 on success
 */
int sendTweet() {
  
  if (twitter.post(tweet)) {
    int status = twitter.wait(&Serial);
    if (status == 200) {
      Serial.println("OK.");
      return 0;
    } else {
      Serial.print("failed : code ");
      Serial.println(status);
      return status;
    }
  } else {
    Serial.println("connection failed.");
  }

}

void setup() {
  
  float initialTemp;

  delay (1000);
  
  // Initial LCD Display
  lcd.begin(20, 4);
  lcd.clear();
  lcd.print("Current Temperatures");
  
  // Ethernet and Serial port for debugging
  initEthernet();  
  Serial.begin(9600);

  // Start reading temps
  sensors.begin();
  
  // Initial Tweet and LCD Set up
  initialTemp = sensors.getTempCByIndex(0);
  delay (5000);
  initialTemp = sensors.getTempCByIndex(0); // Initial values are usually wrong, so need to let it warm up.
  
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print(initialTemp);
  lcd.print((char)223);
  lcd.print("C");
  lastTempC= millis();
  
  lcd.setCursor(0,2);
  lcd.print("Min: ");
  lcd.print(minTempC);
  lcd.print((char)223);
  lcd.print("C");
  
  lcd.setCursor(0,3);
  lcd.print("Max: ");
  lcd.print(maxTempC);
  lcd.print((char)223);
  lcd.print("C");

  sprintf(tweet, "TweetABrew: Started, initial temp: ");
  dtostrf(initialTemp, 3, 1, tweet + strlen(tweet));
  strcpy(tweet + strlen(tweet), "C");
  
  sendTweet();
  
}

/*
 * loop()
 *
 * Reads temps at intervals set by tempInterval. If the temperature is out of the desired
 * fermenting temperature range, sends a WARNING tweet. Otherwise, tweets the temperature based
 * on the tweetInterval. Will not give a warnign tweet twice, unless its longer than the 
 * warnInterval.
 */
void loop() {
  
  unsigned long currentMillis = millis();
  float currentTemp = sensors.getTempCByIndex(0);
  int i;
  
  /* Get temp and update LCD */
  if (currentMillis - lastTemp > tempInterval) {
    
     sensors.requestTemperatures();
    
     lcd.setCursor(0, 1);
     // print the number of seconds since reset:
     lcd.print("Current: ");
     lcd.print(currentTemp);
     lcd.print((char)223);
     lcd.print("C   ");

    /* Check if its warning */
    if ( (currentTemp < minTempC || currentTemp > maxTempC) && (lastTemp < minTempC || lastTemp > maxTempC) ) { 
      
      sprintf(tweet, "TweetABrew: Warning Temperature out of range: ");
      dtostrf(currentTemp, 3, 1, tweet + strlen(tweet));
      strcpy(tweet + strlen(tweet), "C");
      
      Serial.println(currentTemp);
      Serial.println(tweet);
      
      sendTweet();
      
      lastWarn = currentMillis;
      lastTweet = currentMillis;
    }
    /* Check if its time to tweet */
    else if (currentMillis - lastTweet > tweetInterval) {
      
      sprintf(tweet, "TweetABrew: Current Temperature is at: ");
      dtostrf(currentTemp, 3, 1, tweet + strlen(tweet));
      strcpy(tweet + strlen(tweet), "C");
      
      Serial.println(currentTemp);
      Serial.println(tweet);
      
      sendTweet();
      
      lastTweet = currentMillis;
    }
    else {
      // All is good, stay quiet.
    }
 
 
    lastTemp = currentMillis;
  } // if temp interval
  
  lastTempC = currentTemp;
 
  // No need to constantly check 
  delay(1000);

}
