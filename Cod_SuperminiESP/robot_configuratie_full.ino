#include <WiFi.h>
#include <PubSubClient.h>
#include <PCF8574.h>
#include <Wire.h>

#define IN1 5
#define IN2 6
#define IN3 7
#define IN4 10

#define TRIG 4
#define ECHO 3

#define PWM_FREQ 1000
#define PWM_RES  8

#define BASE_SPEED 90
#define MAX_SPEED  255
#define MIN_SPEED  0

#define TIMP_INTRE_DETECTII 3000
#define TIMP_VIRAJ          400

#define KP       0
#define KI       1
#define KD       2
#define ERR_ACC  3
#define ERR_PREV 4

/*const char* WIFI_SSID     = "DESKTOP-LEHEGDT 1521";
const char* WIFI_PASSWORD = "rA34478*";
const char* MQTT_BROKER   = "192.168.137.1";
const int   MQTT_PORT     = 1883;*/

const char* WIFI_SSID     = "iPhone - Sanda";
const char* WIFI_PASSWORD = "georgi2002";
const char* MQTT_BROKER   = "172.20.10.2";
const int   MQTT_PORT     = 1883;

//const char* TOPIC_COMENZI    = "restaurant/comenzi";
const char* TOPIC_COMENZI = "robot/1/comanda";
const char* TOPIC_RASPUNSURI = "restaurant/raspunsuri";
const char* TOPIC_TELEMETRIE = "restaurant/telemetrie";

WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);
PCF8574      pcf(0x38);

bool robotActiv          = false;
int  masaDestinatie      = 0;
int  marcajeNumarate     = 0;
bool bifurcatieDetectata = false;
bool modBifurcatie       = false;

String codTraseu  = "";
int    pozitieCod = 0;

// Decizia curenta:
// 1 = citeste marcaj si mergi inainte
// 2 = citeste marcaj si opreste-te
// 3 = citeste bifurcatie si mergi drept
// 4 = citeste bifurcatie si vireaza
int decizieCurenta = 1;

unsigned long ultimaTelemetrie       = 0,
              ultimaCitireSenzori    = 0,
              ultimaCitireUltrasonic = 0,
              timerViraj             = 0,
              timpPornire            = 0,
              timpUltimaDetectie     = 0,
              timpIntrareModBif      = 0;

double errorCurenta     = 0;
int    leftSpeedCurent  = 0;
int    rightSpeedCurent = 0;
float  distantaObstacol = 100;

byte senzori = 0;

double pid[5];

void pidInit(double Kp, double Ki, double Kd) {
  pid[KP] = Kp; pid[KI] = Ki; pid[KD] = Kd;
  pid[ERR_ACC] = 0; pid[ERR_PREV] = 1;
}

double pidCompute(double error) {
  pid[ERR_ACC] += error;
  double dErr = error - pid[ERR_PREV];
  double out  = pid[KP] * error + pid[KI] * pid[ERR_ACC] + pid[KD] * dErr;
  pid[ERR_PREV] = error;
  return out;
}

byte citesteSenzori() {
  byte val = 0;
  val |= (pcf.digitalRead(2) & 1) << 0;  // R2 - bit 0
  val |= (pcf.digitalRead(3) & 1) << 1;  // R1 - bit 1
  val |= (pcf.digitalRead(5) & 1) << 2;  // C  - bit 2
  val |= (pcf.digitalRead(6) & 1) << 3;  // L1 - bit 3
  val |= (pcf.digitalRead(7) & 1) << 4;  // L2 - bit 4
  return val;
}

double calculeazaEroare() {
  if (!modBifurcatie) {
    // Mod principal - centrat pe R1, folosind C, R1, R2 (biti 2,1,0)
    if      ((senzori & 0b00111) == 0b011 )   return 1;
    else if ((senzori & 0b00111) == 0b101) { pid[ERR_ACC] = 0; return 0; }
    else if ((senzori & 0b00111) == 0b110)    return -1;
    else                                      return 0;
  } else {
    // Mod bifurcatie - centrat pe L1, folosind L2, L1, C (biti 4,3,2)
    if      ((senzori & 0b11100) == 0b01100)   return  1;
    else if ((senzori & 0b11100) == 0b10100) { pid[ERR_ACC] = 0; return 0; }
    else if ((senzori & 0b11100) == 0b11000)   return -1;
    else                                       return 0;
  }
}

void avanseazaCod() {
  if (pozitieCod >= (int)codTraseu.length()) return;

  char c = codTraseu[pozitieCod];

  if (c == 'm') {
    decizieCurenta = (pozitieCod + 1 >= (int)codTraseu.length()) ? 2 : 1;
    pozitieCod++;
  } else if (c == 'b') {
    decizieCurenta = (pozitieCod + 1 < (int)codTraseu.length() && codTraseu[pozitieCod + 1] == 'm') ? 4 : 3;
    pozitieCod++;
  }

  Serial.print("Decizie curenta: "); Serial.println(decizieCurenta);
}

void setMotors(int leftSpeed, int rightSpeed) {
  ledcWrite(IN3, leftSpeed  <= 0 ? -leftSpeed  : 0);
  ledcWrite(IN4, leftSpeed  >  0 ?  leftSpeed  : 0);
  ledcWrite(IN1, rightSpeed <= 0 ? -rightSpeed : 0);
  ledcWrite(IN2, rightSpeed >  0 ?  rightSpeed : 0);
}
String mesaj = "";
void onMesajPrimit(char* topic, byte* payload, unsigned int length) {
  mesaj = "";
  for (unsigned int i = 0; i < length; i++) mesaj += (char)payload[i];

  Serial.print("[MQTT] Comanda primita: ");
  Serial.println(mesaj);

  if (mesaj.startsWith("robot du-te la masa")) {
    int idxCod = mesaj.indexOf(" cod ");
    int masa = mesaj.substring(20, idxCod > 0 ? idxCod : mesaj.length()).toInt();
    codTraseu = idxCod > 0 ? mesaj.substring(idxCod + 5) : "";
    codTraseu.trim();

    if (masa > 0 && codTraseu.length() > 0) {
      masaDestinatie      = masa;
      marcajeNumarate     = 0;
      pozitieCod          = 0;
      bifurcatieDetectata = false;
      modBifurcatie       = false;
      robotActiv          = true;
      timpPornire         = millis();
      timpUltimaDetectie  = millis();
      avanseazaCod();

      String raspuns = "am plecat spre masa " + String(masaDestinatie) + " cod=" + codTraseu;
      mqtt.publish(TOPIC_RASPUNSURI, raspuns.c_str());
      Serial.println("[MQTT] " + raspuns);
    }
  } else if (mesaj == "robot stai") {
    robotActiv = false; bifurcatieDetectata = false; modBifurcatie = false;
    leftSpeedCurent = 0; rightSpeedCurent = 0; errorCurenta = 0;
    setMotors(0, 0);
    mqtt.publish(TOPIC_RASPUNSURI, "m-am oprit");
  } else if (mesaj == "robot status") {
    String status = robotActiv
      ? "in miscare, poz=" + String(pozitieCod) + "/" + String(codTraseu.length()) +
        " decizie=" + String(decizieCurenta) + " cod=" + codTraseu +
        " mod=" + String(modBifurcatie ? "BIF" : "PRINC")
      : "stau pe loc la masa " + String(masaDestinatie);
    mqtt.publish(TOPIC_RASPUNSURI, status.c_str());
  }
}

void conectareWiFi() {
  Serial.print("\n\n\nConectare Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("Conectat! IP: "); Serial.println(WiFi.localIP());
}

void conectareMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Conectare MQTT...");
    if (mqtt.connect("esp32-robot")) {
      Serial.println(" OK!");
      mqtt.subscribe(TOPIC_COMENZI);
      mqtt.publish(TOPIC_RASPUNSURI, "robot online si gata de comenzi");
    } else {
      Serial.print(" eroare cod="); Serial.println(mqtt.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  ledcAttach(IN1, PWM_FREQ, PWM_RES);
  ledcAttach(IN2, PWM_FREQ, PWM_RES);
  ledcAttach(IN3, PWM_FREQ, PWM_RES);
  ledcAttach(IN4, PWM_FREQ, PWM_RES);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pidInit(60.0, 5, 20);

  conectareWiFi();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(onMesajPrimit);
  conectareMQTT();

  Wire.begin(8, 9);
  Wire.setClock(400000);
  pcf.begin();
  pcf.pinMode(2, INPUT);
  pcf.pinMode(3, INPUT);
  pcf.pinMode(5, INPUT);
  pcf.pinMode(6, INPUT);
  pcf.pinMode(7, INPUT);

  Serial.println("LINE FOLLOWER PID + MQTT READY");
}

void loop() {
  if (!mqtt.connected()) conectareMQTT();
  mqtt.loop();

  unsigned long acumSenzori = millis();
  if (acumSenzori - ultimaCitireSenzori >= 12) {
    ultimaCitireSenzori = acumSenzori;
    senzori = citesteSenzori();
  }

  unsigned long acumUltrasonic = millis();
  if (acumUltrasonic - ultimaCitireUltrasonic >= 100) {
    ultimaCitireUltrasonic = acumUltrasonic;
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
    long durata = pulseIn(ECHO, HIGH, 30000);
    distantaObstacol = durata * 0.034 / 2;
  }

  unsigned long acumTelemetrie = millis();
  if (acumTelemetrie - ultimaTelemetrie >= 1000) {
    ultimaTelemetrie = acumTelemetrie;
    String tel = "activ=" + String(robotActiv ? "DA" : "NU") +
                 "\nmasa=" + String(masaDestinatie) +
                 "\ncod=" + codTraseu + " poz=" + String(pozitieCod) + "(" + codTraseu[pozitieCod - 1] + ")" +
                 "\ndecizie=" + String(decizieCurenta) +
                 "\nmod=" + String(modBifurcatie ? "BIF" : "PRINC") +
                 "\nerr=" + String(errorCurenta, 2) +
                 "\nL=" + String(leftSpeedCurent) +
                 "\nR=" + String(rightSpeedCurent) +
                 "\ndist=" + String(distantaObstacol, 1) +
                 "\nsenzori=0b" + String(senzori, BIN)+
                "\n"+String(mesaj);
    mqtt.publish(TOPIC_TELEMETRIE, tel.c_str());
  }

  if (!robotActiv) {
    timpPornire = 0;
    setMotors(0, 0);
    return;
  }

  bool marcajDetectat       = (((~senzori & 0b111  ) == 0b111  ) ||
                               ((~senzori & 0b1110 ) == 0b1110 ) ||
                               ((~senzori & 0b11100) == 0b11100)) &&
                               timpPornire && (millis() - timpUltimaDetectie > TIMP_INTRE_DETECTII);
  bool intersectieDetectata = (~senzori & 0b10000) &&
                              !(marcajDetectat) &&
                              (millis() - timpUltimaDetectie > TIMP_INTRE_DETECTII);

  if (modBifurcatie && (~senzori & 0b00001) && (millis() - timpIntrareModBif > 3000)) {
    modBifurcatie = false;
    pid[ERR_ACC]  = 0;
    Serial.println("MOD: bifurcatie -> principal");
  }

  double error = 0;
  switch (decizieCurenta) {

    case 1:  // marcaj + continua
      if (marcajDetectat) {
        marcajeNumarate++;
        timpPornire        = millis();
        timpUltimaDetectie = millis();
        avanseazaCod();
      }
      error = calculeazaEroare();
      break;

    case 2:  // marcaj + oprire
      if (marcajDetectat) {
        setMotors(0, 0);
        robotActiv = false; bifurcatieDetectata = false; modBifurcatie = false;
        leftSpeedCurent = 0; rightSpeedCurent = 0; errorCurenta = 0;
        timpUltimaDetectie = millis();
        String raspuns = "am ajuns la masa " + String(masaDestinatie);
        mqtt.publish(TOPIC_RASPUNSURI, raspuns.c_str());
        //pu ca sa apaar in telemetrie
        mesaj = "[TRIMIS]: " + raspuns; // Ca să apară în telemetrie pe ecran
        return;
      }
      error = calculeazaEroare();
      break;

    case 3:  // bifurcatie + drept
      if (intersectieDetectata) {
        timpUltimaDetectie = millis();
        avanseazaCod();
      }
      error = calculeazaEroare();
      break;

    case 4:  // bifurcatie + vireaza
      if (intersectieDetectata) {
        modBifurcatie      = true;
        timpIntrareModBif  = millis();
        timpUltimaDetectie = millis();
        avanseazaCod();
        error = 0;
      } else error = calculeazaEroare();
      break;
  }

  if (distantaObstacol > 0 && distantaObstacol < 30) {
    setMotors(0, 0);
    pid[ERR_ACC]  = 0;
    pid[ERR_PREV] = 0;
  } else {
    if(!error) pid[ERR_ACC] = 0;
    double correction = pidCompute(error);
    int leftSpeed  = constrain((int)(BASE_SPEED - correction), millis() - timerViraj > TIMP_VIRAJ ? MIN_SPEED : 60, MAX_SPEED);
    int rightSpeed = constrain((int)(BASE_SPEED + correction), millis() - timerViraj > TIMP_VIRAJ ? MIN_SPEED : 60, MAX_SPEED);
    setMotors(leftSpeed, rightSpeed);
    errorCurenta     = error;
    leftSpeedCurent  = leftSpeed;
    rightSpeedCurent = rightSpeed;
  }
  delay(1);
}