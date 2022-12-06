// LED 
#define LED_BUILTIN 2
#define _DICT_KEYLEN 64
#define _DICT_VALLEN 256

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#define SDA 21
#define SCL 22

// Servo
#include <ESP32Servo.h>
Servo myservo;  // create servo object to control a servo
int posVal = 0;    // variable to store the servo position
int servoPin = 15; // Servo motor pin

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wpa2.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <ESP_Mail_Client.h>
#include <Dictionary.h>
#include <time.h>
#include <TimeLib.h>
#include<ESP32Time.h>
#include <DS1307RTC.h> 

//wifi information
const char* ssid = "UPIoT"; 
const char* password = "Letsdoit9(";

//api information
String openNewYorkTimesApiKey = "eYdANDJ9hiqptHSOAGnj9oChRHgyGzfl";
String listName = "combined-print-and-e-book-fiction";
String offset = "0";

// email information
String SMTPServer = "smtp.office365.com";
int SMTPPort = 587;

String senderEmail = "anabelhilerio@hotmail.com";
String senderPassword = "Awesomesauce0876";
String recipientEmail = "anabelhilerio@hotmail.com";

// THE DEFAULT TIMER IS SET TO 10 SECONDS FOR TESTING PURPOSES
// For a final application, check the API call limits per hour/minute to avoid getting blocked/banned
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;
String jsonBuffer = "{}";


SMTPSession smtp;
ESP_Mail_Session session;

LiquidCrystal_I2C lcd(0x27, 16, 2);


// formats the date
void digitalClockDisplay(){
  printDigits(day());
  Serial.print("/");
  printDigits(month());
  Serial.print("/");
  date += String(year());
  Serial.println();
}

void printDigits(int digits){
  if(digits < 10)
    date += ('0');
  date += String(digits);
}

void processSyncMessage() {
 // if time sync available from serial port, update time and return true
 while (Serial.available() >= TIME_MSG_LEN ) { // time message consists of header & 10 ASCII digits
  char c = Serial.read() ;
  Serial.print(c);
  if ( c == TIME_HEADER ) {
    time_t pctime = 0;
    for (int i = 0; i < TIME_MSG_LEN - 1; i++) {
      c = Serial.read();
      if ( c >= '0' && c <= '9') {
        pctime = (10 * pctime) + (c - '0') ; // convert digits to a number
      } 
    }
    setTime(pctime); // Sync Arduino clock to the time received on the serial port
  }
 }
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}";
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void openNewYorkTimesCall(){
    Dictionary *newBookList = new Dictionary(15);

    // Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      String serverPath = "http://api.nytimes.com/svc/books/v3/lists.json?list=" + listName + "&published-date=" + publishedDate + "&api-key=" + openNewYorkTimesApiKey; //+ " HTTP/1.1"
      
      jsonBuffer = httpGETRequest(serverPath.c_str());
      JSONVar myObject = JSON.parse(jsonBuffer);
      Serial.println(jsonBuffer);
      
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
      }
      else {
        Serial.println("Parsing input succeeded");
      }
      
      int i;
      int j;
      String valueKey = "";
      String val = "";

      //adding values to the dictionary and makes comparison about the values
      if(firstTime){
        for(i = 0; i < 15; i++){
          initalBookList->insert(myObject["results"][i]["book_details"][0]["title"], myObject["results"][i]["book_details"][0]["author"]);
        }
      }
      else{
        for(i = 0; i < 15; i++){
          newBookList->insert(myObject["results"][i]["book_details"][0]["title"], myObject["results"][i]["book_details"][0]["author"]);
          for(j = 0; j < 15; j++){
            if(!(newBookList->key(i).equals(initalBookList->key(j)))){
              valueKey = newBookList->key(i);
              val = newBookList->value(i);
              finalBookList->insert(valueKey, val); 
            }
            else{
              finalBookList.remove(j);
            }
          }
        }
        initalBookList = newBookList;
        free(newBookList);
      }
    }
    else {
      Serial.println("WiFi Disconnected");
    }
}

void emailBookList(){
  session.server.host_name = SMTPServer;
  session.server.port = SMTPPort;
  session.login.email = senderEmail;
  session.login.password = senderPassword;
  session.login.user_domain = "";

  //Declare the message class
  SMTP_Message message;

  message.sender.name = "ESP32";
  message.sender.email = senderEmail;
  message.subject = "Newest New York Times Bestselling Books!";
  message.addRecipient("Anabel Hilerio", recipientEmail);

  //Send HTML message
  String textMsg = "<html><body><h1>Hello! Here are the newest New York Times Best Selling books!</h1><ul>";
  String valueKey = "";
  String val = "";

  //emails the list of books to the user
  if(firstTime){
    for(int i = 0 ; i < initalBookList->count(); i++){
      valueKey = initalBookList->key(i);
      val = initalBookList->value(i);
      textMsg += "<li>" + valueKey + " by " + val + "</li>";
    }
  }
  else{
    if(finalBookList->count() > 0){
      for(int i = 0 ; i < finalBookList->count(); i++){
        valueKey = finalBookList->key(i);
        val = finalBookList->value(i);
        textMsg += "<li>" + valueKey + " by " + val + "</li>";
      }
    }
  }
  textMsg += "</ul></body></html>";
  message.html.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  boolean result = smtp.connect(&session);
  if(!result){
    Serial.println("Error result: " + result);
    //Serial.println("Error connecting: " + smtp.status);
    return;
  }

  if(!MailClient.sendMail(&smtp, &message)){
    Serial.println("Error sending Email, " + smtp.errorReason());
  }

 Serial.println("Message was sent to " + recipientEmail);
}

void LCD_print_string(){
  if(firstTime){
    lcd.print(initalBookList->count());
    lcd.print(" new books!");
  }
  else{
    lcd.print(finalBookList->count());
    lcd.print(" new books!");
  }  
}


// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Wire.begin(SDA, SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
    
  Serial.begin(115200); //Set the baud rate to 115200 for port otherwise wont see anything

  WiFi.disconnect(true);
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)ssid, strlen(ssid));
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)password, strlen(password));
  esp_wifi_sta_wpa2_ent_enable();
  WiFi.begin(ssid);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  boolean firstTime = true;
  Dictionary *initalBookList = new Dictionary(15);
}

 // the loop function runs over and over again forever
 // runs the code on a loop once a day 
 void loop(){
  Dictionary *finalBookList = new Dictionary(15);
  String publishedDate = "";
  if (Serial.available())
  {
    processSyncMessage();
  }
  
  publishedDate = digitalClockDisplay();
  
  openNewYorkTimesCall();
  if(firstTime || (finalBookList->count() > 0)){
    digitalWrite(LED_BUILTIN, HIGH);
    LCD_print_string();
    emailBookList();
    delay(30000);
    digitalWrite(LED_BUILTIN, LOW);
    lcd.clear();
    firstTime = false;
  }
  free(finalBookList);
  delay(86400000);
 }
