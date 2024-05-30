#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <DHT.h>

#define DHTPIN D3      // Definisci il pin a cui è collegato il sensore
#define DHTTYPE DHT11  // DHT 11
DHT dht(DHTPIN, DHTTYPE);

//NodeMCU 1.0 (ESP-12E Module) come scheda
// Definisci il nome della tua rete WiFi e la password
const char* ssid = "SIUMNELKEKKIN";
const char* password = "siumkeksium";

// Variabili per memorizzare i dati del sensore
String temperaturaMax;
String umiditaMax;
String temperaturaMin;
String umiditaMin;
String timer;

// Inizializza la comunicazione seriale software sui pin D1 (GPIO5) e D2 (GPIO4)
SoftwareSerial mySerial(D3, D4);  // RX, TX

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  dht.begin();
  delay(100);

  // Connettiti alla rete WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connessione a ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connessione WiFi stabilita");
  Serial.print("Indirizzo IP: ");
  Serial.println(WiFi.localIP());

  // Effettua la richiesta HTTP iniziale
  WiFiClient client;
  HTTPClient http;
  String url = "http://192.168.186.41:8000/api/Sensor/info?id_cellar=1&id_sensor=1";  // Sostituisci con l'URL della tua pagina web

  if (http.begin(client, url)) {
    int httpCode = http.GET();

    // Se la richiesta ha avuto successo, salva i dati del sensore nelle variabili
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(payload);

      // Crea un documento JSON
      StaticJsonDocument<512> doc;

      // Analizza il payload
      DeserializationError error = deserializeJson(doc, payload);

      // Verifica se l'analisi è andata a buon fine
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      // Ottieni i dati del sensore
      temperaturaMax = doc["Parametri del sensore"]["TemperaturaMax"].as<String>();
      umiditaMax = doc["Parametri del sensore"]["UmiditaMax"].as<String>();
      temperaturaMin = doc["Parametri del sensore"]["TemperaturaMin"].as<String>();
      umiditaMin = doc["Parametri del sensore"]["UmiditaMin"].as<String>();
      timer = doc["Parametri del sensore"]["Timer"].as<String>();
    } else {
      Serial.println("Errore nella richiesta HTTP");
    }

    http.end();
  } else {
    Serial.println("Impossibile connettersi al server");
  }
  delay(1000);
  // Invia i dati del sensore all'Arduino Uno tramite la porta seriale software
  mySerial.print(temperaturaMax);
  mySerial.print("*");
  delay(500);
  mySerial.print(umiditaMax);
  mySerial.print("*");
  delay(500);
  mySerial.print(temperaturaMin);
  mySerial.print("*");
  delay(500);
  mySerial.print(umiditaMin);
  mySerial.print("*");
  delay(500);
  mySerial.print(timer);
  mySerial.println("$$##$$");
  delay(500);
}

void loop() {
  // Leggi i dati del sensore DHT11
// ...
// Leggi i dati del sensore DHT11
float h = dht.readHumidity();
float t = dht.readTemperature();

// Verifica se la lettura del sensore è andata a buon fine
if (isnan(h) || isnan(t)) {
  Serial.println("Errore nella lettura del sensore DHT11");
  return;
}

// Arrotonda i valori di umidità e temperatura a interi
int hInt = round(h);
int tInt = round(t);

Serial.print("Umidità relativa: ");
Serial.print(hInt);
Serial.print(" %\t");
Serial.print("Temperatura: ");
Serial.print(tInt);
Serial.println(" °C");

// Controlla se i valori di umidità e temperatura non rispettano i valori massimi e minimi
if (hInt > umiditaMax.toInt() || hInt < umiditaMin.toInt() || tInt > temperaturaMax.toInt() || tInt < temperaturaMin.toInt()) {
  // Invia un report al server
  WiFiClient client;
  HTTPClient http;
  String url = "http://192.168.186.41:8000/api/Monitoring/report";  // Sostituisci con l'URL del tuo server

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  // Crea un documento JSON con i dati del sensore
  StaticJsonDocument<200> doc;
  doc["Temperatura"] = tInt;  // Usa i valori interi
  doc["Umidita"] = hInt;      // Usa i valori interi
  doc["Peso"] = 10;           // Aggiungi il valore del peso
  doc["id_sensor"] = 1;       // Aggiungi l'id del sensore, sostituisci con il valore corretto

  // Serializza il documento JSON
  String payload;
  serializeJson(doc, payload);

  // Invia i dati al server
  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    Serial.println("Report inviato con successo");
  } else {
    Serial.println("Errore nell'invio del report");
  }

  http.end();
  
    delay(timer.toInt() * 1000);
}
// ...

  

  delay(timer.toInt() * 1000);  // Attendi il tempo specificato dal timer prima di effettuare la prossima lettura
}
