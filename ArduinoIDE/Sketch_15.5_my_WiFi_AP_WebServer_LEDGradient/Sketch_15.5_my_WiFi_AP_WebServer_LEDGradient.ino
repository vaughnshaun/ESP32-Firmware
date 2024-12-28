/**********************************************************************
  Filename    : WiFi AP
  Description : Set ESP32 to open an access point
  Auther      : www.freenove.com
  Modification: 2024/06/20
**********************************************************************/
#include <WiFi.h>
#include <WebServer.h>
#include <sstream>
#include <cstdlib>

const byte ledPins[] = {4, 2, 15};    //define led pins
const byte chns[] = {0, 1, 2};        //define the pwm channels

const char *ssid_AP     = "RouterName"; //Enter the router name
const char *password_AP = "Password"; //Enter the router password
const String toggleLed = "toggleLed";

IPAddress local_IP(192,168,1,100);//Set the IP address of ESP32 itself
IPAddress gateway(192,168,1,10);   //Set the gateway of ESP32 itself
IPAddress subnet(255,255,255,0);  //Set the subnet mask for ESP32 itself

// Create WebServer instance on port 80
WebServer server(80);

long hexToLong(const char* hexString) 
{ 
  char* end; 
  long result = std::strtol(hexString, &end, 16);
  return result;
}

String createAjaxRequest(String funcName, String method = "GET")
{
  String html = "";
  html += "function "+ funcName +"() {";
  html += "  fetch('/"+ funcName +"', {method: '"+method+"'})";
  html +=     ".then(response => response);";
  html += "}";

  return html;
}

String createOnClick(String funcName)
{
  // Get the button element
  String html = "const button = document.getElementById('"+funcName+"');";

  // Add an event listener to the button
  html += "button.onclick = function(event) {";

    html += funcName + "();";
    // Prevent the default action (which could be a redirect)
    html += "event.preventDefault();";

  html += "};";

  return html;
}

String onChangeFavColor()
{
  // Get the button element
  String html = "const button = document.getElementById('favcolor');";

  // Add an event listener to the button
  html += "button.onchange = function(event) {";
    html += "  fetch('/favcolor', {method: 'POST', headers: {'Content-Type': 'text/plain'}, body: button.value.replace('#', '') })";
    html +=     ".then(response => response);";
    

  html += "};";

  return html;
}

String onInputFavColor()
{
  // Get the button element
  String html = "const button = document.getElementById('favcolor');";

  // Add an event listener to the button

  html += "let timeout = 0;";
  html += " button.oninput = function(event) { ";
  html += " clearTimeout(timeout); ";
    html += " timeout = setTimeout(function() { ";
        html += "  fetch('/favcolor', {method: 'POST', headers: {'Content-Type': 'text/plain'}, body: event.target.value.replace('#', '') })";
        html +=     ".then(response => response);";
  html += " }, 100);";

  html += "};";

  return html;
}

void setColor(long rgb) {
  Serial.printf("Long integer: %ld\n", rgb); 
  ledcWrite(ledPins[0], 255 - (rgb >> 16) & 0xFF);
  ledcWrite(ledPins[1], 255 - (rgb >> 8) & 0xFF);
  ledcWrite(ledPins[2], 255 - (rgb >> 0) & 0xFF);
}

// Function to handle the root page
void handleRoot() {
  String html = "<html><body style='zoom: 1.5'>";
  html += "<label style='font-size: 30px;'>Light Color </label> ";
  html += "<div style='width: 200px;height: 50px; display: inline-block; position: relative; top: 10px; overflow: hidden; border-style: solid; border-width: 2px; border-color: gray;'>";
  html += "<input type='color' id='favcolor' name='favcolor' value='#ff0000' style='width: 600px;height: 500px;position: absolute; top: -40px; left: -200px;' >";
  html += "</div>";
  html += "<script>";
  html += onInputFavColor();
  html += "</script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);  // Send the HTML page to the client
}

// Function to toggle the LED
void handleToggle() {
  Serial.println("Button Pressed");
  server.send(200); // always send 200 success to prevent default redirect
  //digitalWrite(ledPin, !digitalRead(ledPin));  // Toggle the LED state
  //server.sendHeader("Location", "/");  // Redirect back to root page
  //server.send(303);  // HTTP response code for redirect
}

void handleColor() {
  if (server.hasArg("plain")) 
  { 
    String body = server.arg("plain"); 
    // Print the received plain text to the serial monitor 
    Serial.printf("Received text: %s\n", body.c_str()); 

    setColor(hexToLong(body.c_str()));

    // Send response 
    server.send(200, "text/plain", "Data received"); 
  } 
  else 
  { 
    server.send(400, "text/plain", "No body provided"); 
  }
}

void setup(){
  Serial.begin(115200);

  for(int i = 0; i < 3; i++) {   //setup the pwm channels
    ledcAttachChannel(ledPins[i], 1000, 8, chns[i]);
  }

  delay(2000);
  Serial.println("Setting soft-AP configuration ... ");
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
  Serial.println("Setting soft-AP ... ");
  boolean result = WiFi.softAP(ssid_AP, password_AP);
  if(result){
    Serial.println("Ready");
    Serial.println(String("Soft-AP IP address = ") + WiFi.softAPIP().toString());
    Serial.println(String("MAC address = ") + WiFi.softAPmacAddress().c_str());
  }else{
    Serial.println("Failed!");
  }
  Serial.println("Setup End");

   // Start the server
  server.begin();
  Serial.println("HTTP server started");

  // Define server routes
  server.on("/", handleRoot);       // Handle root URL
  server.on("/" + toggleLed, handleToggle);  // Handle /toggle URL
  server.on("/favcolor", HTTP_POST, handleColor);
}
 
void loop() {
  server.handleClient();  // Handle client requests
}
