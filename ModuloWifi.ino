#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <DHT.h>

#define DHTPIN D3      // il pin a cui è collegato il sensore
#define DHTTYPE DHT11  // DHT 11
DHT dht(DHTPIN, DHTTYPE);

//NodeMCU 1.0 (ESP-12E Module) come scheda
//  rete WiFi e la password
const char* ssid = "SIUMNELKEKKIN";
const char* password = "siumkeksium";

String UrlServer = "http://192.168.125.41:8000/api/"; 
int id_sensor = 1;//il sensore deve essere creato dall'utente dall'app prima
int id_cellar = 1;
// Variabili per memorizzare i dati del sensore
int temperaturaMax;
int umiditaMax;
int temperaturaMin;
int umiditaMin;
int timer;
bool erroreLettura;
bool erroreValori;

void setup() {
  Serial.begin(9600);
  erroreValori = false;
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
  String url = UrlServer + "Sensor/info?id_cellar=" + String(id_cellar) + "&id_sensor=" + String(id_sensor);

  if (http.begin(client, url)) {
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
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

      // Verifica se il documento JSON contiene i campi richiesti
      if (!doc.containsKey("Parametri del sensore")) {
        Serial.println("Il documento JSON non contiene il campo 'Parametri del sensore'");
        erroreValori = true;
        return;
      }

      JsonObject parametriDelSensore = doc["Parametri del sensore"];
      if (!parametriDelSensore.containsKey("TemperaturaMax") || !parametriDelSensore.containsKey("UmiditaMax") || !parametriDelSensore.containsKey("TemperaturaMin") || !parametriDelSensore.containsKey("UmiditaMin") || !parametriDelSensore.containsKey("Timer")) {
        Serial.println("Il campo 'Parametri del sensore' non contiene uno dei campi richiesti");
        erroreValori = true;
        return;
      }

      // Ottieni i dati del sensore
      temperaturaMax = parametriDelSensore["TemperaturaMax"].as<int>();
      umiditaMax = parametriDelSensore["UmiditaMax"].as<int>();
      temperaturaMin = parametriDelSensore["TemperaturaMin"].as<int>();
      umiditaMin = parametriDelSensore["UmiditaMin"].as<int>();
      timer = parametriDelSensore["Timer"].as<int>();
    } else {
      Serial.print("Errore nella richiesta HTTP, codice di stato: ");
      Serial.println(httpCode);
      erroreValori = true;
      return;
    }

    http.end();
  } else {
    Serial.println("Impossibile connettersi al server");
  }
  delay(1000);
}


void loop() {
  // Verifica se le variabili sono state impostate
  if (erroreValori == true) {
    Serial.println("Variabili non impostate, ritorno...");
    setup();
    return;
  }
  erroreLettura = false;
  int intervallo = 3000;  // Controlla la temperatura ogni 3 secondo
  int tempoTotale = 0;
  //faccio un controllo periodico dato che arduino non supporta il multitread
  while (tempoTotale < timer * 1000) {
    // Leggi i dati del sensore DHT11
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Verifica se la lettura del sensore è andata a buon fine
    if (isnan(h) || isnan(t)) {

      Serial.println("Errore nella lettura del sensore DHT11");
    }

    // Arrotonda i valori di umidità e temperatura a interi
    int hInt = round(h);
    int tInt = round(t);

    // Invia i dati al server periodicamente


    // Controlla se i valori di umidità e temperatura non rispettano i valori massimi e minimi
    if (tInt > temperaturaMax || tInt < temperaturaMin || hInt > umiditaMax || hInt < umiditaMin) {
      Serial.println("Errore nei valori dell sensore");
      // Invia un report al server
      if (erroreLettura == false) {
        inviaDati(tInt, hInt);
        erroreLettura = true;
        Serial.println("invio errore");
      }
    } else if (erroreLettura == true) {
      inviaDati(tInt, hInt);
      erroreLettura = false;
      Serial.println("invio dopo errore");
    } else if (tempoTotale == 0 && erroreLettura == false) {
      inviaDati(tInt, hInt);
      Serial.println("invio periodico");
    }


    delay(intervallo);  // Attendi l'intervallo prima di effettuare la prossima lettura
    tempoTotale += intervallo;
  }
}

void inviaDati(int temperatura, int umidita) {
  WiFiClient client;
  HTTPClient http;
  String url = UrlServer + "Monitoring/report";

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  // Crea un documento JSON con i dati del sensore
  StaticJsonDocument<200> doc;
  doc["Temperatura"] = temperatura;  // Usa i valori interi
  doc["Umidita"] = umidita;          // Usa i valori interi
  doc["id_sensor"] = id_sensor;      // Aggiungi l'id del sensore, sostituisci con il valore corretto
  doc["Peso"] = 15;                  // il peso è fisso perchè non è ancora stata implementata una cella di carico

  // Serializza il documento JSON
  String payload;
  serializeJson(doc, payload);

  // Invia i dati al server
  int httpCode = http.POST(payload);
  String response = http.getString();
  if (httpCode == HTTP_CODE_OK) {

    Serial.println("Dati inviati con successo: " + response + "payload" + payload);
  } else {
    Serial.println("Errore nell'invio dei dati: " + response + "payload" + payload);
  }

  http.end();
}
