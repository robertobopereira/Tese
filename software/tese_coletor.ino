#include <dht22.h>
#include <Wire.h>
#include "RTClib.h"
#include <SD.h>
#include <SoftwareSerial.h>

#define div_tensao 3
#define maximovalor -9999
#define minimovalor 9999
#define Reset_AVR() wdt_enable(WDTO_30MS); while(1) {}

dht22 DHT;
RTC_DS1307 RTC;

SoftwareSerial gps =  SoftwareSerial(7,6);

unsigned int amostra=0,media=0,contadoramostra[9];
byte dump=0,beep=0,auxint=0,auxint2=0,auxint3=0,listasensor[9][2];
long tempoamostra=0,tempomedia=0,tempoatual=0;
double valorsensor[9][6],auxdouble=0;
DateTime relogio;
char buffer[32],buffer2[12],auxchar;
File arquivo;

void tocar(){
  tone(beep,3000); 
  delay(150);
  noTone(beep);
  delay(80);   
}

int freeRam() {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

char lergps(){
  while(1) if(gps.available()) return gps.read(); 
}

char erro(){
 while(1){
  if (beep!=0) { 
      tocar(); 
   }
   delay(100);
 } 
}

void setup(){
  Serial.begin(9600);
  Serial.println(F("Iniciando o Sistema"));
  if (!SD.begin(8)) {
    Serial.println(F("Falha na leitura do cartao ou sem cartao!"));
    erro(); // Validar =====================================================>
  }
  // Iniciando SD para pegar arquivo de configuracao
  arquivo = SD.open("config.cfg",FILE_READ);
  if (!arquivo) {
    Serial.println(F("Nao existe arquivo de configuracao"));
    erro(); // Validar =====================================================>
  }
  auxint=0;
  auxint2=0;
  auxint3=0;
  // Processando arquivo 
  while (arquivo.available()) {
    auxchar=arquivo.read();
    if(auxchar==35) while(arquivo.read()!=10);
    if(auxchar==64){
      // Localizado inicio de comando
      auxint=0;
      auxint2=0;
      while((auxchar=arquivo.read())!=10){
        if(auxchar==61){ 
          buffer[auxint]='\0'; 
          auxint=0; 
          auxint2=1;
        }
        else {
          if(auxint2==0){ 
            buffer[auxint]=(char) auxchar; 
            auxint++;
          }
          else{ 
            buffer2[auxint]=(char) auxchar; 
            auxint++;
          }
        }
      }
      buffer2[auxint]='\0';
      // Iniciando interpretacao dos dados
      if (strcmp(buffer, "DUMP")==0){
        dump=atoi(buffer2);
      } 
      else if (strcmp(buffer, "AMOSTRA")==0){ 
        amostra=atoi(buffer2);
      }
      else if (strcmp(buffer, "MEDIA")==0){ 
        media=atoi(buffer2);
      }
      else if (strcmp(buffer, "BEEP")==0){ 
        beep=atoi(buffer2);
        pinMode(beep,OUTPUT);
      }     
      else if (strcmp(buffer, "SENSOR")==0){
        //processar os tipos de sensores
        for(auxint=0;auxint<5;auxint++) buffer[auxint]=buffer2[auxint];
        buffer[auxint]='\0';
        buffer2[0]=buffer2[6];
        buffer2[1]='\0';
        if (strcmp(buffer, "analo")==0){ 
          listasensor[auxint3][0]=1; 
          listasensor[auxint3][1]=atoi(buffer2);
          auxint3++;
        }
        else if(strcmp(buffer, "dht22")==0){ 
          listasensor[auxint3][0]=2; 
          listasensor[auxint3][1]=atoi(buffer2); 
          auxint3++;
        }
        else if(strcmp(buffer, "gpskm")==0){ 
          listasensor[auxint3][0]=4; 
          //listasensor[auxint3][1]=atoi(buffer2); 
          gps.begin(9600);
          auxint3++;
        }
        else if(strcmp(buffer, "divte")==0){ 
          listasensor[auxint3][0]=5; 
          listasensor[auxint3][1]=3; 
          auxint3++;
        }
      }
    }
  }
  arquivo.close();
  // Iniciando dispositivos
  Wire.begin();
  RTC.begin();

  auxint2=0;
  for(auxint=0;auxint<9;auxint++) if(listasensor[auxint][0]==4) { auxint2=auxint; }// Procurando GPS
  if (auxint2>0){ //GPS Encontrado
    gps.flush();
    auxint=0;
    auxint2=0;
    auxint3=0;
    auxdouble=0;
    Serial.println(F("Configurando GPS"));
    gps.println(F("$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0*28"));
    while(1){
      auxchar=lergps();
      if(auxchar=='$'){
        buffer[0]=lergps();
        buffer[1]=lergps();
        buffer[2]=lergps();
        buffer[3]=lergps();
        buffer[4]=lergps();
        buffer[5]='\0';
        auxchar=lergps();
        if((strcmp(buffer,"GPGGA")==0) && auxint2==0){
          Serial.print(F("."));
          // $GPGGA,083559.00,3723.2475,N,12158.3416,W,1,07,1.0,9.0,M,,M,,0000*18
          auxint=0;
          while((auxchar=lergps())!=','){}
          while((auxchar=lergps())!=','){
            buffer[auxint]=auxchar;
            auxint++;
          }
          buffer[auxint]='\0';
          auxdouble=atof(buffer);
          Serial.print(F("."));
          if(auxdouble!=8960) auxint2=1;
        }
        if((strcmp(buffer,"GPZDA")==0) && auxint2==1){
          // $GPZDA,082710.00,04,07,2002,00,00*60
          //        01234567890123456789012345
          auxint=0;
          Serial.println(F("Sincronizando data"));
          while((auxchar=lergps())!='*'){
            buffer[auxint]=auxchar;
            auxint++;
          }
          buffer2[0]=buffer[0];
          buffer2[1]=buffer[1];
          buffer2[2]='\0';
          relogio.hh=atoi(buffer2);
          buffer2[0]=buffer[2];
          buffer2[1]=buffer[3];
          buffer2[2]='\0';
          relogio.mm=atoi(buffer2);
          buffer2[0]=buffer[4];
          buffer2[1]=buffer[5];
          buffer2[2]='\0';
          relogio.ss=atoi(buffer2);
          buffer2[0]=buffer[11];
          buffer2[1]=buffer[12];
          buffer2[2]='\0';
          relogio.d=atoi(buffer2);
          buffer2[0]=buffer[14];
          buffer2[1]=buffer[15];
          buffer2[2]='\0';
          relogio.m=atoi(buffer2);
          buffer2[0]=buffer[17];
          buffer2[1]=buffer[18];
          buffer2[2]=buffer[19];
          buffer2[3]=buffer[20];
          buffer2[4]='\0';
          relogio.yOff=atoi(buffer2);
          relogio.yOff-=2000;
          RTC.adjust(relogio);
          sprintf(buffer,"%04d-%02d-%02d %02d:%02d:%02d", relogio.year(), relogio.month(), relogio.day(),relogio.hour(),relogio.minute(),relogio.second() );
          break;
        }
      } 
    }
  }
  if (! RTC.isrunning()) {
    Serial.println(F("Problema com o relogio"));
    erro();
  } 
  else {
    Serial.println(F("Iniciando o Relogio"));
  }
  if (dump==1) Serial.println(F(" ===== Modo Dump Ativo ===="));
  // Zerando acumuladores
  for(auxint=0;auxint<9;auxint++){
    valorsensor[auxint][0]=0;
    valorsensor[auxint][1]=maximovalor;
    valorsensor[auxint][2]=minimovalor;
    valorsensor[auxint][3]=0;
    valorsensor[auxint][4]=maximovalor;
    valorsensor[auxint][5]=minimovalor;
    contadoramostra[auxint]=0;
  }
  if (beep!=0) { 
    delay(500); 
    tocar(); 
  }
  tempoatual = (millis()/1000);
  tempoamostra = tempoatual;
  tempomedia = tempoatual;
}

void loop(){
  tempoatual=(millis()/1000);
  if((tempoatual-tempomedia)>=media){
    if (dump==1){
      arquivo = SD.open("dump.csv",FILE_WRITE);
      arquivo.println("===================");
      arquivo.close();
    }
    relogio = RTC.now();
    sprintf(buffer, "%02d%02d%02d.csv", relogio.month(), relogio.day(), relogio.hour());
    arquivo = SD.open(buffer,FILE_WRITE);
    sprintf(buffer,"%04d-%02d-%02d %02d:%02d:%02d;", relogio.year(), relogio.month(), relogio.day(),relogio.hour(),relogio.minute(),relogio.second() );
    arquivo.print(buffer);
    Serial.println(F("Media"));
    for(auxint=0;auxint<9;auxint++){
      switch(listasensor[auxint][0]){
      case 1:
        arquivo.print(valorsensor[auxint][0]/contadoramostra[auxint]);
        arquivo.print(";");
        valorsensor[auxint][0]=0;
        valorsensor[auxint][1]=maximovalor;
        valorsensor[auxint][2]=minimovalor;
        contadoramostra[auxint]=0;
        break;
      case 2:
        arquivo.print(valorsensor[auxint][0]/contadoramostra[auxint]);
        arquivo.print(";");
        arquivo.print(valorsensor[auxint][1]);
        arquivo.print(";");
        arquivo.print(valorsensor[auxint][2]);
        arquivo.print(";");
        arquivo.print(valorsensor[auxint][3]/contadoramostra[auxint]);
        arquivo.print(";");
        arquivo.print(valorsensor[auxint][4]);
        arquivo.print(";");
        arquivo.print(valorsensor[auxint][5]);
        arquivo.print(";");
        valorsensor[auxint][0]=0;
        valorsensor[auxint][1]=maximovalor;
        valorsensor[auxint][2]=minimovalor;
        valorsensor[auxint][3]=0;
        valorsensor[auxint][4]=maximovalor;
        valorsensor[auxint][5]=minimovalor;
        contadoramostra[auxint]=0;        
        break;
      case 4:
        auxint2=1;
        while(auxint2){
          gps.flush();
          auxchar=lergps();
          if(auxchar=='$'){
            buffer[0]=lergps();
            buffer[1]=lergps();
            buffer[2]=lergps();
            buffer[3]=lergps();
            buffer[4]=lergps();
            buffer[5]='\0';
            auxchar=lergps();
            if(strcmp(buffer,"GPGGA")==0){ 
              auxint3=1;
              while(auxint3){
                auxchar=lergps();
                if(auxchar==',') arquivo.print(';');
                else if((auxchar=='*') || (auxchar=='$')){
                  auxint3=0;
                  auxint2=0;
                }
                else arquivo.print(auxchar);
              }
            }
          }
        }
        break;
      case 5:
        arquivo.print(valorsensor[auxint][0]/contadoramostra[auxint]);
        arquivo.print(";");
        valorsensor[auxint][0]=0;
        contadoramostra[auxint]=0;
        break;
      } 
    }
    arquivo.println();
    arquivo.close();
    tempomedia=tempoatual;
    if (beep!=0) { 
      tocar(); 
    }
    Serial.println(freeRam()-202);
  }
  if((tempoatual-tempoamostra)>=amostra){
    // faz coleta individual
    if(dump==1){
      arquivo = SD.open("dump.csv",FILE_WRITE);
      relogio = RTC.now();
      sprintf(buffer,"%04d-%02d-%02d %02d:%02d:%02d;", relogio.year(), relogio.month(), relogio.day(),relogio.hour(),relogio.minute(),relogio.second() );
      arquivo.print(buffer);
    }    
    Serial.println(F("Coletando"));
    for(auxint=0;auxint<9;auxint++){
      switch(listasensor[auxint][0]){
      case 1:
        auxdouble=analogRead(listasensor[auxint][1]);
        valorsensor[auxint][0]+=auxdouble;
        contadoramostra[auxint]++;
        if(auxdouble>valorsensor[auxint][1]) valorsensor[auxint][1]=auxdouble;
        if(auxdouble<valorsensor[auxint][2]) valorsensor[auxint][2]=auxdouble;
        if(dump==1){ 
          arquivo.print(auxdouble);
          arquivo.print(";"); 
        }
        break;
      case 2:
        auxint2=0;
        auxint3=0;
        while(auxint2<2) {
          if((DHT.read(listasensor[auxint][1]))==0) { auxint3=1; auxint2=3; }
          else auxint2++;
        }
        if(auxint3==1){
          auxdouble=DHT.temperature;
          valorsensor[auxint][0]+=auxdouble;
          contadoramostra[auxint]++;
          if(auxdouble>valorsensor[auxint][1]) valorsensor[auxint][1]=auxdouble;
          if(auxdouble<valorsensor[auxint][2]) valorsensor[auxint][2]=auxdouble;
          if(dump==1){ 
            arquivo.print(auxdouble);
            arquivo.print(";"); 
          }
          auxdouble=DHT.humidity;
          valorsensor[auxint][3]+=auxdouble;
          if(auxdouble>valorsensor[auxint][4]) valorsensor[auxint][4]=auxdouble;
          if(auxdouble<valorsensor[auxint][5]) valorsensor[auxint][5]=auxdouble;
          if(dump==1){ 
            arquivo.print(auxdouble);
            arquivo.print(";"); 
          }
        } 
        else {
          Serial.print(F("Falha na leitura do sensor DHT porta:"));
          Serial.println(listasensor[auxint][1]);
          if (beep!=0) for (auxint2=0;auxint2<listasensor[auxint][1];auxint2++){
            tocar();
          }
          if(dump==1){ 
            arquivo.print(F("xx;xx;"));
          }
        }
        break;
      case 5:
        auxdouble=analogRead(listasensor[auxint][1]);
        auxdouble=( 5 * auxdouble ) / 326;
        valorsensor[auxint][0]+=auxdouble;
        contadoramostra[auxint]++;
        if(auxdouble<=7.5) { 
          if (beep!=0) tocar(); 
          Serial.print(F("Verificar bateria, tensao:")); 
          Serial.println(auxdouble);
        };
        if(dump==1){ 
          arquivo.print(auxdouble);
          arquivo.print(";"); 
        }
        break;
      }
    }
    arquivo.println();
    arquivo.close();
    tempoamostra=tempoatual;
  }
}

