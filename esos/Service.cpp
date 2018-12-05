#include "Service.h"

Sim800 simServer = Sim800(Serial2, APN, USERNAME, PASSWORD, BASIC_AUTH);;

const char slpserver[] = SERVER;
const char slpuri[] = REQ_STR;
const char istserver[] = IST_SERVER;
const char isturi[] = POSTREQ;

uint8_t temp = 0;

File dir;
File reader;

void ServiceBegin()
{
  printString(SERVICE_CHECK, APN);
  uint8_t temp = simServer.begin();
  if (temp)
    printSystemLog(SUCCESSFULL, SIM_800, SERVICE_OK);
  else
    printSystemLog(SUCCESS_ERROR, SIM_800 , SERVICE_ERROR);
}

void sendRequestString(double *externalHum,
                       double *externalTemp,
                       double *internalTemp,
                       double *light_intensity,
                       double *windSpeed,
                       double *windDirection,
                       double *rainFall,
                       double *pressure,
                       double *soilMoisture,
                       double *altitude,
                       double *waterLevel,
                       double *battry,
                       int type,
                       String TimeStamp,
                       String Guid)
{
  String req;
  if (type == ISTSOS_REQUEST)
  {

    req = String(Guid);
    req.concat(";");
    req += TimeStamp;

    if(WL_ENABLE){
      req.concat(",");
      req.concat(*waterLevel);
    }else{
      req.concat(",");
      req.concat(*internalTemp);
  
      req.concat(",");
      req.concat(*soilMoisture);
  
      req.concat(",");
      req.concat(*light_intensity / 1000);
  
      req.concat(",");
      req.concat(*pressure / 1000);
  
      req.concat(",");
      req.concat(*externalHum);
  
      req.concat(",");
      req.concat(*externalTemp);
  
      req.concat(",");
      req.concat(*rainFall);
  
      req.concat(",");
      req.concat(*windDirection);
  
      req.concat(",");
      req.concat(*windSpeed);  
    }

    // write to temp log
    writeFileSD(ISTSOS_MEM_LOG, getFileNameTime() , req);

    // read and send
    readAndSend(ISTSOS_MEM_LOG, ISTSOS_DATA_LOG,istserver, isturi,true);
    
  }
  else if (type == SLPIOT_REQUEST)
  {

    req = String(F("{"));
    req.concat("\"GUID\":\"");
    req += Guid;
    req.concat("\",");
    req.concat("\"dt\":\"");
    req += TimeStamp;
    req.concat("\",");
    req.concat("\"H\":\"");
    req.concat(*externalHum);
    req.concat("\",");
    req.concat("\"TE\":\"");
    req.concat(*externalTemp);
    req.concat("\",");
    req.concat("\"L\":\"");
    req.concat(*light_intensity);
    req.concat("\",");
    req.concat("\"TI\":\"");
    req.concat(*internalTemp);
    req.concat("\",");
    req.concat("\"WS\":\"");
    req.concat(*windSpeed);
    req.concat("\",");
    req.concat("\"WD\":\"");
    req.concat(*windDirection);
    req.concat("\",");
    req.concat("\"RG\":\"");
    req.concat(*rainFall);
    req.concat("\",");
    req.concat("\"P\":\"");
    req.concat(*pressure);
    req.concat("\",");
    req.concat("\"SM\":\"");
    req.concat(*soilMoisture);
    req.concat("\",");
    req.concat("\"AT\":\"");
    req.concat(*altitude);
    req.concat("\",");
    req.concat("\"WL\":\"");
    req.concat(*waterLevel);
    req.concat("\",");
    req.concat("\"BV\":\"");
    req.concat(*battry);
    req.concat("\"}");

    // write to data file in not send state
    writeFileSD(SLPIOT_MEM_LOG, getFileNameTime(), req);

    // read files and send
    readAndSend(SLPIOT_MEM_LOG, SLPIOT_DATA_LOG,slpserver, slpuri,false);
  }
  else
  {
    printSystemLog(GEN_ERROR,PARAMETER_ERROR);
  }
}

void readAndSend(String temp_folder,String log_folder,char server[], char uri[], bool auth){

  String req;
  dir = SD.open(temp_folder);
 
  while(true){
    reader = dir.openNextFile();
    if (!reader)
      break;
    
    printString(temp_folder, reader.name());
    req = readFileSD(temp_folder, reader.name());

    Serial.println(req);

    if (sendRequstMessage(server,uri,req, auth) == SEND_SUCCESS){
      printString(SEND_SUCCESSFULL, reader.name());
      writeFileSD(log_folder, getFileNameDate(), req);
      removeFile(temp_folder, reader.name());
    }else{
      printString(SEND_ERROR, reader.name());
      break;  
    }
  }
}

uint8_t sendRequstMessage(char server_url[], char url[], String message, bool auth) {

  uint8_t count = ERROR_REPEATE_COUNT;
  String server = String(server_url);
  printSystemLog(SENDING, server);

  while (count > 0) {
    uint8_t tmp = auth == true ? simServer.executePost(server_url, url, message) : simServer.executePostPure(server_url, url, message);
    // network failiur
    if (tmp == REQUEST_SUCCESS) {
      printSystemLog(SUCCESSFULL, server, DATA_SEND_SUCCESSFULLY);
      return SEND_SUCCESS;
    } else if (tmp == NETWORK_FAILURE) {
      printSystemLog(GEN_ERROR, NETWORK_FAILURE, DATA_SEND_ERROR);
      return SEND_ERROR;
    } else if (tmp == GPRS_FAILURE) {
      printSystemLog(GEN_ERROR, GPRS_FAILURE, DATA_SEND_ERROR);
      return SEND_ERROR;
    } else {
      printSystemLog(GEN_ERROR, REMOTE_ERROR, DATA_SEND_ERROR);
    }
    printSystemLog(RESENDING, server);
    count--;
  }

  printSystemLog(SEND_ERROR, server, DATA_SEND_ERROR);
  return SEND_ERROR;
}

DateTime ntpUpdate()
{
  uint32_t *result = simServer.ntpUpdate(NTP_UPDATE_URL, 0);
  uint8_t tmp = 5;
  long last = millis();
  if (result == 0)
    tmp = 0;
  while (tmp > 0 && result[0] < 2018)
  {
    result = simServer.ntpUpdate(NTP_UPDATE_URL, 0);
    delay(100);
    if ((millis() - last) > 10000UL) {
      tmp = 0;
      break;
    }

  }
  if (tmp == 0)
  {
    DateTime dt = DateTime(2017, 01, 01, 00, 00, 00);
    return dt;
  }

  DateTime dt = DateTime((uint16_t)result[0], (uint8_t)result[1], (uint8_t)result[2], (uint8_t)result[3], (uint8_t)result[4], (uint8_t)result[5]);
  dt = dt + TimeSpan(0, 5, 30, 0);
  return dt;
}

int readRSSI()
{
  return simServer.readRSSI();
}
