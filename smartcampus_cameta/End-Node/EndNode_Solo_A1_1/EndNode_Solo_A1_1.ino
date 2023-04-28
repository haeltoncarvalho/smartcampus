/**
 * Código para os endnodes que realizarão as coletas na UFPA Cametá/Tocantins.
 * São 4 tipos diferentes para 7 dispositivos.
 * Biblioteca de siglas por coleta de sensores;
 *  tmp, umd, xma, sonic
 * d -> payload
 * i -> id da dojot
 * t -> temperatura
 * u -> umidade
 * l -> luminosidade
 * o -> umidadeSolo
 * v -> vibração
 * p -> presença (PIR)
 * f -> chama
 * s -> som
 * a -> gás álcool (MQ-3)
 * m -> gás metano (MQ-4)
 * g -> gás GLP  (MQ-5)
 * c -> gás Monóxido de Carbono (MQ-9)
 * x -> gás tóxico  (MQ-135)
 * r -> rssi (LoRa) 
 */

#include <SPI.h>
#include <RH_RF95.h>
#include "DHT.h"
#define DHTTYPE DHT11 // DHT 11

// Variáveis

int DHTPIN = A1;
int LIGHTPIN = 0;
int FLAMEPIN = 1;
int VIBRPIN = 3;
int SOLOHPIN = 4;

int deviceId = 1;
boolean vibracao;
float temp, humid, luz, chama, soloUmi, vibra, rssi;
char tem_1[8] = {"\0"}, hum_1[8] = {"\0"}, luz_1[8] = {"\0"}, chama_1[8] = {"\0"}, umidadeSolo_1[8] = {"\0"}, vibracao_1[8] = {"\0"}, rssi_1[8] = {"\0"}, deviceId_1[6] = {"\0"};
char *node_id = "<16a>"; // From LG01 via web Local Channel settings on MQTT.Please refer <> dataformat in here.
uint8_t datasend[85];
unsigned int count = 1;

DHT dht(DHTPIN, DHTTYPE); // Inicia Biblioteca

/** Variáveis LoRa**/
RH_RF95 rf95;
float frequency = 915.0;

void setup()
{
  Serial.begin(9600);
  // while (!Serial) ; // Wait for serial port to be available
  configPin();
  configLoRa();
}

void configPin()
{
  pinMode(LIGHTPIN, INPUT);
  pinMode(FLAMEPIN, INPUT);
  pinMode(SOLOHPIN, INPUT);
  pinMode(VIBRPIN, INPUT);
  pinMode(DHTPIN, INPUT);
  dht.begin();
}

void configLoRa()
{
  Serial.println("Start Setup LoRa Client");
  if (!rf95.init())
    Serial.println("init failed");
  // Setup ISM frequency
  rf95.setFrequency(frequency);
  // Setup Power,dBm
  rf95.setTxPower(13);
  // Setup Spreading Factor (6 ~ 12)
  rf95.setSpreadingFactor(7);
  // Setup BandWidth, option: 7800,10400,15600,20800,31250,41700,62500,125000,250000,500000
  // Lower BandWidth for longer distance.
  rf95.setSignalBandwidth(125000);
  // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8)
  rf95.setCodingRate4(5);
  rf95.setSyncWord(0x34);
}

void loop()
{
  dhtTempHumid();
  valor_chama();
  isVibration();
  soloHSensor();
  valor_luz();
  // Debug para visualizar dados coletados na porta serial;
  printSerial();
  // Manipulação dos dados coletados para envio via LoRa;
  sensorWrite();
  // Envio de fato dos dados utilizando LoRa;
  SendDataLoRa();
}

void dhtTempHumid()
{
  temp = dht.readTemperature();
  humid = dht.readHumidity();
  delay(3000);
}

void soloHSensor()
{
  soloUmi = analogRead(SOLOHPIN);
  soloUmi = map(soloUmi, 550, 0, 0, 100);
  delay(1000);
}

void valor_luz()
{
  luz = analogRead(LIGHTPIN);
  delay(1000);
}

void valor_chama()
{
  chama = analogRead(FLAMEPIN);
  delay(1000);
}
void isVibration()
{
  vibracao = digitalRead(VIBRPIN);
  delay(500);
}

void SendDataLoRa()
{
  Serial.println(F("Sending data to LG01"));
  rf95.send((char *)datasend, sizeof(datasend));
  rf95.waitPacketSent();

  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf95.waitAvailableTimeout(3000))
  {
    if (rf95.recv(buf, &len))
    {

      Serial.println((char *)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
      rssi = rf95.lastRssi();
    }
    else
    {
      Serial.println("recv failed");
    }
  }
  else
  {
    Serial.println("No reply, is LoRa server running?");
  }
  delay(3000);
}

void printSerial()
{
  Serial.print("###########    ");
  Serial.print("COUNT=");
  Serial.print(count);
  Serial.println("    ###########");
  count++;
  Serial.println(F("Temperatura e Umidade:"));
  Serial.print("[");
  Serial.print(temp);
  Serial.print("℃");
  Serial.print(",");
  Serial.print(humid);
  Serial.print("%");
  Serial.print("]");
  Serial.println("");
  Serial.print("Umidade do Solo : ");
  Serial.print(soloUmi);
  Serial.println("%");
  Serial.print("valor luz: ");
  Serial.println(luz);
  Serial.print("valor chama: ");
  Serial.println(chama);
  if (vibracao == 1)
  {
    vibra = 1;
    Serial.println("Vibracao: SIM ");
  }
  else if (vibracao == 0)
  {
    vibra = 0;
    Serial.println("Vibracao: NÃO ");
  }
}

void sensorWrite()
{
  Serial.println("Inicio sensor write:");
  char data[85] = {};
  for (int i = 0; i < 85; i++)
  {
    data[i] = node_id[i];
  }
  dtostrf(temp, 0, 1, tem_1);
  dtostrf(humid, 0, 1, hum_1);
  dtostrf(chama, 0, 1, chama_1);
  dtostrf(luz, 0, 1, luz_1);
  dtostrf(soloUmi, 0, 1, umidadeSolo_1);
  itoa(vibra, vibracao_1, 10);
  dtostrf(rssi, 0, 1, rssi_1);
  itoa(deviceId, deviceId_1, 10);

  Serial.println("debugInicial:");
  Serial.println(tem_1);
  Serial.println(hum_1);
  Serial.println(chama_1);
  Serial.println(vibracao_1);
  Serial.println(umidadeSolo_1);
  Serial.println(luz_1);
  strcat(data, "{\"d\":");
  strcat(data, "{\"i\":");
  strcat(data, deviceId_1);
  strcat(data, ",\"t\":");
  strcat(data, tem_1);
  strcat(data, ",\"u\":");
  strcat(data, hum_1);
  strcat(data, ",\"f\":");
  strcat(data, chama_1);
  strcat(data, ",\"v\":");
  strcat(data, vibracao_1);
  strcat(data, ",\"o\":");
  strcat(data, umidadeSolo_1);
  strcat(data, ",\"l\":");
  strcat(data, luz_1);
  strcat(data, ",\"r\":");
  strcat(data, rssi_1);
  strcat(data, "}}");
  strcpy((char *)datasend, data);

  Serial.println("debug pacote: ");
  Serial.println((char *)datasend);
  Serial.println(sizeof datasend);
}