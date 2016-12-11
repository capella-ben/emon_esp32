/*
 *  This sketch reports on the power usage on the mains and solar lines
 *
 *  By Ben Jackson
 */

#include <WiFi.h>
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "nvs.h"
#include "nvs_flash.h"


/* -------------------------------------------------------------------- 
*    CONSTANTS 
*  -------------------------------------------------------------------- */
#define ADC_BITS    12
#define ADC_COUNTS  (1<<ADC_BITS)
const float ICAL = 90.9;            // calibration factor
const int ledPin = 5;               // onboard LED for ESP32 Thing
const int mainsPin = 36;            // ADC for mains power CT
const int solarPin = 37;            // ADC for solar power CT
const static char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
//#define DEBUG

/* -------------------------------------------------------------------- 
*    GLOBALS 
*  -------------------------------------------------------------------- */
char* ssid;
char* password;
char* sIp;
char* sNetmask;
char* sGateway;
double filteredI = 0;
double last_filtered_value;
double offsetI, sqI, sumI, Irms;
int sample = 0;
struct netconn *conn, *newconn;
err_t err;



//const static char http_index_html[] = "<html><head><title>Congrats!</title></head><body><h1>Welcome to our lwIP HTTP server!</h1><p>This is a small test page, served by httpserver-netconn.</body></html>";



/* -------------------------------------------------------------------- 
*    FUNCTIONS 
*  -------------------------------------------------------------------- */


char* readConfigValue(nvs_handle handle, char* key) {

  size_t required_size;
  nvs_get_str(handle, key, NULL, &required_size);    //find the size of the value
  char* valueOut = (char*)malloc(required_size);       //set the size of the value
  nvs_get_str(handle, key, valueOut, &required_size);   //get the value

  return valueOut;
}




double calcIrms(unsigned int Number_of_Samples, int pin)
{

  for (unsigned int n = 0; n < Number_of_Samples; n++)
  {
    sample = analogRead(pin);

    // Digital low pass filter extracts the 2.5 V or 1.65 V dc offset,
    //  then subtract this - signal is now centered on 0 counts.
    offsetI = (offsetI + (sample-offsetI)/1024);
    filteredI = sample - offsetI;

    // Root-mean-square method current
    // 1) square current values
    sqI = filteredI * filteredI;
    // 2) sum
    sumI += sqI;

    #ifdef DEBUG      //debug generates the table of output so you can plot a graph
    Serial.print(n);
    Serial.print("\t");
    Serial.print(sample);
    Serial.print("\t");
    Serial.print(filteredI);
    Serial.print("\t");
    Serial.println(offsetI);
    #endif
  
  
  }

  double I_RATIO = ICAL *((3300/1000.0) / (ADC_COUNTS));
  Irms = I_RATIO * sqrt(sumI / Number_of_Samples);

  #ifdef DEBUG      // summary var output
  Serial.println();
  Serial.print("I_RATIO ");
  Serial.println(I_RATIO);
  Serial.print("Irms:   ");
  Serial.println(Irms);
  Serial.print("Power:  ");
  Serial.println(Irms*240);
  #endif
  
  sumI = 0;

  return Irms;
}


/*
 * Once a connection is found, this reads the connection headers and responds.
 */
static void http_server_netconn_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;
  err_t err;

  digitalWrite(ledPin, HIGH);
  
  /* Read the data from the port, blocking if nothing yet there. 
   We assume the request (the part we care about) is in one netbuf */
  err = netconn_recv(conn, &inbuf);
  
  if (err == ERR_OK) {
    netbuf_data(inbuf, (void**)&buf, &buflen);

    /* Is this an HTTP GET command? (only check the first 5 chars, since
    there are other formats for GET, and we're keeping it very simple )*/
    if (buflen>=6 &&
        buf[0]=='G' &&
        buf[1]=='E' &&
        buf[2]=='T' &&
        buf[3]==' ' &&
        buf[4]=='/' &&
        buf[5]==' ' ) {

      #ifdef DEBUG
      Serial.println("  Connection received");
      #endif
      
      /* Send the HTML header 
             * subtract 1 from the size, since we dont send the \0 in the string
             * NETCONN_NOCOPY: our data is const static, so no need to copy it
       */
      netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);

      /* -------------------------------------------
       * This is where the power usage is calculated
         ------------------------------------------- */
      // calculate energy
      double Irms = calcIrms(1480, mainsPin);  // Calculate current 
      double dPower = Irms * 240;

//      String sHTML = String("<html><head><title>openEnergyMonitor</title></head><body>");
      String sHTML = "";
      String sPower = String(dPower, 2);
      sHTML += sPower;
      sHTML += "\tMains Power\r\n";
//      sHTML += "</body></html>";
      
      //convert to char array
      int ilen = sHTML.length() + 1;
      char cHTML[ilen];
      sHTML.toCharArray(cHTML, ilen);

      Serial.println(cHTML);
      
      /* Send our HTML page */
      netconn_write(conn, cHTML, sizeof(cHTML)-1, NETCONN_NOCOPY);
    }
  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);
  
  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
  digitalWrite(ledPin, LOW);

}




void setup()
{
    Serial.begin(115200);
    delay(10);
      
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);

    offsetI = ADC_COUNTS>>1;

    // Read config from NVS
    nvs_handle nvs;
    err = nvs_flash_init();
    if (err != 0) {
       Serial.print("Error initalising NVS. ERROR CODE: ");
       Serial.println(err, HEX);
    }
    err = nvs_open("homeSys", NVS_READWRITE, &nvs);
    if (err != 0) {
      Serial.print("Error opening NVS. ERROR CODE: ");
      Serial.println(err, HEX);
    }
    ssid = readConfigValue(nvs, "wifiSsid");
    password = readConfigValue(nvs, "wifiPwd");
    sIp = readConfigValue(nvs, "ip");
    sNetmask = readConfigValue(nvs, "netmask");
    sGateway = readConfigValue(nvs, "gateway");


    
    // Connect to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    
    //IPAddress local = IPAddress.fromString("192.168.0.152");
    IPAddress local, gateway, subnet;
    local.fromString(sIp);
    gateway.fromString(sGateway);
    subnet.fromString(sNetmask);
    if (WiFi.config(local, gateway, subnet) == false) {
      Serial.println("Error assigning IP Address");
    }

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());

    delay(5000);
    Serial.println("----Ready----");
    

    // Setup the network for the HTTP server
    /* Create a new TCP connection handle */
    conn = netconn_new(NETCONN_TCP);
    /* Bind to port 80 (HTTP) with default IP address */
    netconn_bind(conn, NULL, 80);
    /* Put the connection into LISTEN state */
    netconn_listen(conn);
    
    digitalWrite(ledPin, LOW);

}


void loop()
{
  do {
    digitalWrite(ledPin, LOW);
    err = netconn_accept(conn, &newconn);
    if (err == ERR_OK) {
      http_server_netconn_serve(newconn);
      netconn_delete(newconn);
    }
  } while(err == ERR_OK);
}

