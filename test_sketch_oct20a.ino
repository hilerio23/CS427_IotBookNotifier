// LED 
#define LED_BUILTIN 2
#define _DICT_KEYLEN 64
#define _DICT_VALLEN 256

// Servo
#include <ESP32Servo.h>
Servo myservo;  // create servo object to control a servo
int posVal = 0;    // variable to store the servo position
int servoPin = 15; // Servo motor pin

#include <Arduino.h>
#include <WiFi.h>
//#include <WiFiNINA.h>
#include <esp_wpa2.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <ESP_Mail_Client.h>
#include <Dictionary.h>

const char* ssid = "hilerio1000"; //"UPIoT"; "TheOneAndOnly";
const char* password = "ArchbishopMurphy2021Adrianita"; //"Letsdoit9("; "WeirdChange8&!";

String openNewYorkTimesApiKey = "eYdANDJ9hiqptHSOAGnj9oChRHgyGzfl";
String listName = "combined-print-and-e-book-fiction";
String publishedDate = "2022-10-20";
String offset = "0";

String SMTPServer = "smtp.office365.com";
int SMTPPort = 587;

String senderEmail = "anabelhilerio@hotmail.com";
String senderPassword = "Awesomesauce0876";
String recipientEmail = "anabelhilerio@hotmail.com";

// THE DEFAULT TIMER IS SET TO 10 SECONDS FOR TESTING PURPOSES
// For a final application, check the API call limits per hour/minute to avoid getting blocked/banned
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 10 seconds (10000)
unsigned long timerDelay = 10000;
String jsonBuffer = "{}";


SMTPSession smtp;
ESP_Mail_Session session;

boolean firstTime = true;
//CreateHashMap(bookList, String, String);
//Dictionary<String, String> bookList;
Dictionary *bookList = new Dictionary(15);

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

//String[] openNewYorkTimesCall(){
void openNewYorkTimesCall(){
   // Send an HTTP GET request
  //if ((millis() - lastTime) > timerDelay) {
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
      if(firstTime){
        for(i = 0; i < 15; i++){
          //bookList[myObject["results"][i]["book_details"][0]["author"]] = myObject["results"][i]["book_details"][0]["title"];
          //bookList.set(myObject["results"][i]["book_details"][0]["title"], myObject["results"][i]["book_details"][0]["author"]);
          bookList->insert(myObject["results"][i]["book_details"][0]["title"], myObject["results"][i]["book_details"][0]["author"]);
        }
        //bookKeys String[bookList.length()];
        //for(i = 0; i < bookKeys.length; i++){
        //  bookKeys[i] = myObject["results"][i]["book_details"][0]["title"];
        //}
        firstTime = false;
      }
      else{
        
      }
    
      Serial.print("JSON object = ");
      Serial.println(myObject);
      Serial.print("Title: ");
      Serial.println(myObject["results"][0]["book_details"][0]["title"]);
      Serial.print("Author: ");
      Serial.println(myObject["results"][0]["book_details"][0]["author"]);
    }
    else {
      Serial.println("WiFi Disconnected");
    }

    //return bookKeys;
    //lastTime = millis();
}

void emailBookList(){//String[] bookKeys){
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
  message.addRecipient("Microcontrollerslab", recipientEmail);

  //Send HTML message
  String textMsg = "<html><body><h1>Hello! Here are the newest New York Times Best Selling books!</h1><ul>";
  int size = bookList->count();
  String valueKey = "";
  String val = "";
  
  for(int i = 0 ; i < size; i++){
    //textMsg += bookList[bookKeys[i]] + " by " + bookKeys[i] + "\n";
    valueKey = bookList->key(i);
    val = bookList->value(i);
    textMsg += "<li>" + valueKey + " by " + val + "</li>";
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


// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
    
  Serial.begin(115200); //Set the baud rate to 115200 for port otherwise wont see anything

  //WiFi.disconnect(true);
  //esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)ssid, strlen(ssid));
  //esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)password, strlen(password));
  //esp_wifi_sta_wpa2_ent_enable();
  //WiFi.begin(ssid);
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  //String[] bookKeys = openNewYorkTimesCall();

  openNewYorkTimesCall();
  digitalWrite(LED_BUILTIN, HIGH);
  //emailBookList(bookKeys);
  emailBookList();
  delay(30000);
  digitalWrite(LED_BUILTIN, LOW);


}

 // the loop function runs over and over again forever
 void loop() {
  
    //delay(333);
    //Serial.println(openNewYorkTimesCall());
    
    //emailBookList(bookKeys); 
    //delay(60000);

 // So what else should go here to make the sprinkler work then?

 // So maybe... read the sensor, read OpenWeather, etc tell servo to do someting, if swithc is pressed then do what? etc....
  
 }
