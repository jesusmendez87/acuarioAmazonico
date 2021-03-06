//esp32 shield
//Incluimos la biblioteca

#include <TaskScheduler.h>

#include <WiFi.h>
//#include <WiFiUdp.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <HTTPClient.h>
#include <EEPROM.h>
#include <Wire.h>
#include <TimeLib.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Update.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include "esp_system.h"


unsigned long tiempoAnteriorEncendido = 0;  //guarda tiempo de referencia para comparar
unsigned long tiempoAnteriorApagado = 0;  //guarda tiempo de referencia para comparar
bool estadoValvula = true;
int intcambioAgua;
bool estadoPeristaltica = false;


float cambioAgua;  // guarda veces día cambio agua
int numCambioAgua = 0;
float mlLitro;
float actionAditar;
float litrosCambiados;
boolean boolvvacio, boolvanterior = false; // boolean control cambio agua

unsigned long millisPeristaltica=0;
#define APAGADO 6000L
boolean apagadoPeristaltica;

//// ESTO DEPENDE DE LA PLACA DE RELES //////////
#define OFF HIGH  // Cambiar para logica positiva/negativa 
#define ON LOW   // Cambiar para logica positiva/negativa 

//void upaditivo();  
//Scheduler ts;
//unsigned long ml = 1000; // 1 segundo accionamos la electrovalvula
//Task Tupaditivo(ml, TASK_FOREVER, &upaditivo);

//// CONTADORES MINUTOS ACCIONES RELES ///
unsigned long timeon = 0; // Control tiempo vvacio
unsigned long timeoff = 0; // Control tiempo vvacio
unsigned long tiempomin = 0;// Control tiempo vvacio

unsigned long timeon2 = 0; // Control tiempo vllena
unsigned long timeoff2 = 0; // Control tiempo vllena
unsigned long tiempomin2 = 0;// Control tiempo vllena


unsigned long timeon3 = 0; // Control tiempo peristaltica
unsigned long timeoff3 = 0; // Control tiempo peristaltica
unsigned long tiempomin3 = 0;// Control tiempo peristaltica


//Declaramos pines salida

#define  luzam  14  // rele 220v
#define  luzmd  26 // rele 220v
#define  luzpm  25 // rele 220v
#define  bombasump  17 // rele 220v
#define   vvacio  18  // rele 12v
#define   vllena  19  //rele 12v
#define   peristaltica  27  //rele 12v



//Declaramos-+ pines entrada

#define    boyanivbajo  12  // desconecta bomba sump
#define    boyasup 13 // contect electrovalvula llenado
#define    boyadanger 5 // aviso emergencia




bool botoncambio = false; // boolean para cambios esp_interna
bool botonluz = false ; // boolean para luz esp_interna
String estadofotoperiodo;



void writeString(char add, String data);  // funcion escribir eeprom
String read_String(char add);// cadena leer eeprom


//PIN de temperatura
const int oneWireBus = 23;

OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);


// Datos conexión wifi. usa los tuyos!!!
const char* ssid = "xxxxxxxxxxxxxxxx";
const char* password = "xxxxxxx";
const char* host = "esp32";

//  web server port number to 8080
WebServer server(8080); //WiFiServer server(80)inicia servidor con temperatura / hora / programación
WiFiServer server2(443); //WiFiServer server(443) inicia servidor
// Variable to store the HTTP request
String header;

// WTD

const int loopTimeCtl = 0;
hw_timer_t *timer = NULL;
void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}



//   Login page programacion

const char* loginIndex =
  "<form name='loginForm'>"
  "<table width='20%' bgcolor='A09F9F' align='center'>"
  "<tr>"
  "<td colspan=2>"
  "<center><font size=4><b>ESP32 Login Page</b></font></center>"
  "<br>"
  "</td>"
  "<br>"
  "<br>"
  "</tr>"
  "<td>Username:</td>"
  "<td><input type='text' size=25 name='userid'><br></td>"
  "</tr>"
  "<br>"
  "<br>"
  "<tr>"
  "<td>Password:</td>"
  "<td><input type='Password' size=25 name='pwd'><br></td>"
  "<br>"
  "<br>"
  "</tr>"
  "<tr>"
  "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
  "</tr>"
  "</table>"
  "</form>"
  "<script>"
  "function check(form)"
  "{"
  "if(form.userid.value=='root1234' && form.pwd.value=='admin1234')"
  "{"
  "window.open('/serverIndex')"
  "}"
  "else"
  "{"
  " alert('Error Password or Username')          "
  "}"
  "}"
  "</script>";


// Server Index Page


const char* serverIndex =
  "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
  "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
  "<input type='file' name='update'>"
  "<input type='submit' value='Update'>"
  "</form>"
  "<div id='prg'>progress: 0%</div>"
  "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
  "},"
  "error: function (a, b, c) {"
  "}"
  "});"
  "});"
  "</script>";






// NTP Servers:

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
byte H  ;
byte Min;
byte Seg;
byte dia;

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  // Serial.println(&timeinfo, " %H:%M:%S");
  H = timeinfo.tm_hour;
  Min = timeinfo.tm_min;
  Seg = timeinfo.tm_sec;
  dia = timeinfo.tm_mday;
}





/*
   setup function
*/


void setup() {

  pinMode(luzam, OUTPUT);
  pinMode(luzmd, OUTPUT);
  pinMode(luzpm, OUTPUT);
  pinMode(bombasump, OUTPUT);
  pinMode(vvacio, OUTPUT);
  pinMode(vllena, OUTPUT);
  pinMode (peristaltica, OUTPUT);


  pinMode(boyasup, INPUT_PULLUP);
  pinMode(boyanivbajo, INPUT_PULLUP);
  pinMode(boyadanger, INPUT_PULLUP);

  digitalWrite(luzpm, OFF);
  digitalWrite(luzmd, OFF);
  digitalWrite(luzam, OFF);

  digitalWrite(vvacio, OFF);
  digitalWrite(vllena, OFF);
  digitalWrite(peristaltica, OFF);
  String encendido;
  String apagado;
  String fotoperiodo;
  String horaenc;

  delay(500);
  EEPROM.begin(512);
  sensors.begin();   //Se inicia el sensor de temperatura

  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("Iniciando...");

  delay(1000);


  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  delay(2000);

  if (WiFi.status() != WL_CONNECTED) {
    WiFiManager wifiManager;
    wifiManager.setTimeout(180);

    if (!wifiManager.startConfigPortal("Acuario Red wifi")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(2000);
    }
  }

  Serial.println(WiFi.localIP());
  delay(500);

  // Initialize a NTPClient to get time
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();


  server.begin();
  server2.begin();

  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
    HTTPClient http;
    http.begin("http://192.168.0.198/jmendez/amazonico/fotoperiodo.php"); //Specify the URL donde rescatar configuración de horario,
    //aditivo, cambio de agua
    int httpCode = http.GET();                                        //Make the request

    if (httpCode > 0) { //Check for the returning code
      String fotoperiodo = http.getString();
      String encendido = fotoperiodo.substring(0, 8);
      String apagado = fotoperiodo.substring(9, 16);
      String horaenc = fotoperiodo.substring(0, 2);
      String horaapa = fotoperiodo.substring(9, 11);
      String vaciado = fotoperiodo.substring(18, 20);
      String mlLitroSt = fotoperiodo.substring(21, 26);
      mlLitro = mlLitroSt.toFloat();

      Serial.println(fotoperiodo);
      Serial.println(encendido);
      Serial.println(apagado);
      Serial.println(vaciado);
      Serial.println(mlLitro, 4);
      //lecura eeprom

      String recivedData;
      recivedData = read_String(18);
      delay(1000);
      if (recivedData != fotoperiodo) {
        writeString(18, fotoperiodo);
        Serial.println("Nueva configuración cargada!!");
      }
      else {
        Serial.println("No es necesario actualizar datos!!");
      }
      delay(3000);

    }
    else {
      Serial.println("Error on HTTP request");
    }

    http.end(); //Free the resources
  }


  /*use mdns for host name resolution*/

  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", getdatos());
  });

  server.on("/admin", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */

  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/

      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });


  delay(500);

 // ts.init();
 // ts.addTask(Tupaditivo);
 // Tupaditivo.enable();

  delay(800);


  //wtd

  timer = timerBegin(0, 80, true); //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);
  timerAlarmWrite(timer, 80000000, false); //set time in us 80 segundos
  timerAlarmEnable(timer); //enable interrupt

  Serial.println("Start program.... ");

}


//// Inicio loop /////////////////////////////

void loop() {


  if ( WiFi.status() ==  WL_CONNECTED ) {
    // WiFi is UP,  do what ever
  } else {
    // wifi down, reconnect here
    WiFi.begin(  );
    int WLcount = 0;
    int UpCount = 0;
    while (WiFi.status() != WL_CONNECTED && WLcount < 200 )
    {
      delay( 100 );
      Serial.printf(".");
      if (UpCount >= 60)  // just keep terminal from scrolling sideways
      {
        UpCount = 0;
        Serial.printf("\n");
      }
      ++UpCount;
      ++WLcount;
    }
  }



  sensors.requestTemperatures();   //Se envía el comando para leer la temperatura
  float temp = sensors.getTempCByIndex(0); //Se obtiene la temperatura en ºC

  // variables para control de tiempos
  int estadovllena = digitalRead(vllena ); // Leer estado electrovalvula llenado
  int estadovvacio = digitalRead(vvacio); // Leer estado electrovalvula vacio
  int intPeristaltica = digitalRead(peristaltica);

  /////  contador minutos ultimo cambio de agua vvacio
  if ( estadovvacio == ON) {
    timeon = millis();
  }
  if (estadovvacio == OFF) {
    timeoff = millis();
    tiempomin = (timeoff - timeon) / 1000 / 60; // millis a minutos
  }
  /////  contador minutos Ãºltimo cambio de agua vllena

  if ( estadovllena == ON) {
    timeon2 = millis();
  }
  if (estadovllena == OFF) {
    timeoff2 = millis();
    tiempomin2 = (timeoff2 - timeon2) / 1000 / 60; // millis a minutos
  }

  if (intPeristaltica == ON) {
    timeon3 = millis();
  }
  if (intPeristaltica == OFF) {
    timeoff3 = millis();
    tiempomin3 = (timeoff3 - timeon3) / 1000 / 60; // millis a minutos
  }


  String recivedData;
  recivedData = read_String(18);   /*************(10)*/
  String horainicio = recivedData.substring(0, 2);
  String horaapagado = recivedData.substring(9, 11);
  String minutoinicio = recivedData.substring(3, 5);
  String minutoapagado = recivedData.substring(12, 14);
  String cambioAgua = recivedData.substring(18, 20);
  int horaintinicio;
  horaintinicio = horainicio.toInt();
  int horaintapagado;
  horaintapagado = horaapagado.toInt();
  int minutointinicio;
  minutointinicio = minutoinicio.toInt();
  int minutointapagado;
  minutointapagado = minutoapagado.toInt();
  intcambioAgua = cambioAgua.toInt();

  //calculoCambioAgua(intcambioAgua);


  /////  intervalos cambios de agua
  int estadoboyadanger =  digitalRead(boyadanger);


  int intervaloEncendido = 60000;  // tiempo que esta encendido el led
  int IntervaloApagado = (intcambioAgua * 60000); // tiempo que esta apagado el led

  if ((millis() - tiempoAnteriorEncendido >= intervaloEncendido) && estadoValvula == true) { //si ha transcurrido el periodo programado
    estadoValvula = false; //actualizo la variable para apagar el led
    tiempoAnteriorApagado = millis(); //guarda el tiempo actual para comenzar a contar el tiempo apagado
    Serial.println("apagado");
  }
  if ((millis() - tiempoAnteriorApagado >= IntervaloApagado) && estadoValvula == false) { //si ha transcurrido el periodo programado
    estadoValvula = true; //actualizo la variable para encender el led
    tiempoAnteriorEncendido = millis(); //guarda el tiempo actual para comenzar a contar el tiempo encendido
    Serial.println("encendido");
  }

  if ((estadoValvula == true) || (botoncambio == true) || (estadoboyadanger == HIGH) ) {
    digitalWrite(vvacio, ON);
    estadoPeristaltica = true;

  } else {
    digitalWrite(vvacio, OFF);
    estadoPeristaltica = false;
  }


  boolvanterior = boolvvacio;
  boolvvacio = estadoPeristaltica;
  if (!boolvanterior && boolvvacio) {
    ++numCambioAgua;
  }

  /*
    if ((Min == 20 )&& (Seg>19) || (Min == 40 )&&
    (Seg>19) || (Min == 00) || (estadoboyadanger == HIGH)|| (botoncambio == true) ) {
    digitalWrite(vvacio, ON);
    }
    else {
    digitalWrite(vvacio, OFF);

    }  */

  timerWrite(timer, 0); //reset timer 80 segundos máximo espera
  server.handleClient();
  printLocalTime();
  // inicio servidor 2 efecto tormenta
  WiFiClient client = server2.available();   // listen for incoming clients
  if (client.available()) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("<body style=background-color:black>");

            client.print("<a href=\"/H\"><button>Cambio Agua On</a></button><br><br>");
            client.print("<a href=\"/L\"><button>Cambio Agua Off</a></button><br>");

            client.println("<br>");
            client.print("<a href=\"/ON\"><button>Pantalla On</a></button><br><br>");
            client.print("<a href=\"/OFF\"><button>Pantalla Off</a></button><br>");

            client.println("<br>");
            client.print("<a href=\"/rst\"><button>Reiniciar Sistema</a></button><br><br>");
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
        // Check to see if the client request was "GET /H" or "GET /L":

        if (currentLine.endsWith("GET /H")) {
          botoncambio = true;
        }

        if (currentLine.endsWith("GET /L")) {
          botoncambio = false;
        }

        if (currentLine.endsWith("GET /ON")) {
          botonluz = true;
        }

        if (currentLine.endsWith("GET /OFF")) {
          botonluz =  false;
        }
        if (currentLine.endsWith("GET /rst")) {
          ESP.restart();
        }

        // close the connection:
      }
    }
    client.stop();
    Serial.println("Client Disconnected.");
  }




  if  ((botonluz == true) || ((H == horaintinicio) && (Min >= minutointinicio)) || ((H == horaintapagado) && (Min <= (minutointapagado)))) { //inicio fotoperiodo amanecer
    digitalWrite(luzam, ON);
    estadofotoperiodo = "Amaneciendo";
  }
  else  if  ((H >  horaintinicio) && (H <  horaintapagado)) { //inicio fotoperiodo amanecer
    digitalWrite(luzmd, ON);
    digitalWrite(luzam, ON);
    digitalWrite(luzpm, ON);
    estadofotoperiodo = "Dia";
  }


  else     if  ((H == horaintapagado) && (Min > (minutointapagado))) {
    digitalWrite(luzam, OFF);
    digitalWrite(luzmd, OFF);
    digitalWrite(luzpm, ON);
    estadofotoperiodo = "Anocheciendo";
  }

  else  if  ((H == (horaintapagado + 1)) || (H == (horaintapagado + 2))) {
    estadofotoperiodo = "Anocheciendo";
  }
  else    {
    digitalWrite(luzam, OFF);
    digitalWrite(luzmd, OFF);
    digitalWrite(luzpm, OFF);
    estadofotoperiodo = "off";
  }


  calculoAditivo(); //aditivo a añadir mlLitro
  llenado_(); // LLENADO AGUA POR BOYA
  pumpsump(); // para bomba sump por boya niv minimo

  bool apagarPeris;  
  



   if (millisPeristaltica != 0) {
    if (millis() - millisPeristaltica >= APAGADO) {
      apagarPeris = true;  
      millisPeristaltica = 0L;
      numCambioAgua = 0;
      apagadoPeristaltica = false;
    }
  }
 if (apagadoPeristaltica == true) {
    apagarPeris = false;
    millisPeristaltica = millis();
   }
digitalWrite(peristaltica, apagarPeris);


}


void upaditivo() {
//eliminada la tarea
  
}


String getdatos() {   //String con hora y temperatura para mostrar en servidor web
  int estadoboyainf = digitalRead(boyanivbajo);   //  bomba sump por boya niv minimo
  int estadoboya = digitalRead(boyasup);
  int estadovvacio = digitalRead(vvacio);
  int estadovllena = digitalRead(vllena);
  int estadoboyadanger = digitalRead(boyadanger);
  cambioAgua = calculoCambioAgua(intcambioAgua);
  String separador1 = ";";
  String separador = ":";

  sensors.requestTemperatures();   //Se envía el comando para leer la temperatura
  float temp = sensors.getTempCByIndex(0); //Se obtiene la temperatura en ºC

  return String(estadofotoperiodo + separador1 + H + separador + Min + separador + Seg + separador1 + temp + separador1 +
                tiempomin + separador1 +
                tiempomin2 + separador1 + estadovllena + separador1 + estadovvacio + separador1 + estadoboyainf + separador1 + estadoboya +
                separador1 + estadoboyadanger + separador1 + cambioAgua + separador1 + tiempomin3 + separador1 + actionAditar + separador1 + numCambioAgua);
}



// devuelve los litros al día que cambia
float calculoCambioAgua (int minutosCambio) {
  float litrosCambio = 1.760;  // ml por cambio de agua
  float numeroCambios = 1440 / minutosCambio; // 1440 minutos tiene un día, devuelve número de cambios al día.
  float litrosDia = numeroCambios * litrosCambio;
  return litrosDia;
}

// 1 ml por litro de aditivo acciona la peristaltica.
void calculoAditivo () {
  litrosCambiados = numCambioAgua * 1.760; // 1 ml cada 40 litros = 0.025
  actionAditar = (mlLitro * litrosCambiados);
  if (actionAditar >= 1) { //si superamos 1ml ponemos contador a cero
//   ts.execute();  //disparamos la regla. Aditamos la cantidad establecida de 1mlz
 
   apagadoPeristaltica = true;
    Serial.println("aditamos!");
  }
else {
 apagadoPeristaltica = false;
}
}



void writeString(char add, String fotoperiodo)
{
  int _size = fotoperiodo .length();
  int i;
  for (i = 0; i < _size; i++)
  {
    EEPROM.write(add + i, fotoperiodo[i]);
  }
  EEPROM.write(add + _size, '\0'); //Add termination null character for String Data
  EEPROM.commit();
}


String read_String(char add)
{
  int i;
  char data[100]; //Max 100 Bytes
  int len = 0;
  unsigned char k;
  k = EEPROM.read(add);
  while (k != '\0' && len < 500) //Read until null character
  {
    k = EEPROM.read(add + len);
    data[len] = k;
    len++;
  }
  data[len] = '\0';
  return String(data);
}




void pumpsump() {  // para bomba de sump por boya niv minimo

  int boyainf = digitalRead(boyanivbajo);
  int boyasuper = digitalRead(boyasup);
  if (boyainf == LOW) {
    digitalWrite(bombasump, OFF);

  }
  if (boyasuper == HIGH && boyainf == HIGH) {
    digitalWrite(bombasump, ON);

  }
}


void llenado_() {

  // LLENADO DE AGUA TRAS VACIAR CON APERTURA DE ELECTROVALVULA VVACIO

  int estadoboya = digitalRead(boyasup);
  int estadovvacio = digitalRead(vvacio);
  int estadovllena = digitalRead(vllena);

  if ((estadoboya == ON) && (estadovvacio == OFF)) {
    digitalWrite(vllena, ON);

  } else {
    digitalWrite(vllena, OFF);

  }
}
