#include "Wire.h"

/*******************************************************************/
int fona_read(char *msg, size_t msz, size_t *len)
{
  unsigned char c;

  if(*len >= msz) return(-1);

  while(Serial1.available()) {
    c=(unsigned char)Serial1.read();
//    Serial.println(c);
    if((c != '\r') && (c != '\n')) {
      *(msg+*len)=c;
      (*len)++;
      if(*len >= msz) return(1);
    } else if(c == '\n') return(1);
  }

  return(0);
}

int fona_cmd(char *cmd, char *rsp, size_t rsz)
{
  int rval=-1,ret;
  size_t len;
  char *p,msg[1024];
  

  if(rsz <= 1) return(-1);
  *rsp='\0';
  
  Serial1.write(cmd,strlen(cmd));
  
  len=0;
  for(;;) {
    if((ret=fona_read(msg,sizeof(msg)-1,&len)) == 1) {
      msg[len]='\0';
//      Serial.println(msg);
      if(strcmp(msg,"OK") == 0) break;
      p=msg;
      while(*p) {
        if(isspace(*p)) p++;
        else break;
      }
      if(*p != '\0') {
        strncpy(rsp,p,rsz);
//        Serial.println(msg);
      }
      len=0;
    } else if(ret != 0) goto end;
  }

  rval=0;
  
end:

  return(rval);
}

int fona_init(void)
{
  int rval=-1,setpin=0,ret;
  int cpinok=0,pinready=0,callready=0,smsready=0;
  char *at;
  char msg[1024]={0};
  size_t len;

  Serial.println("fona_init begin");
  
  at=(char*)"AT\n";
  if(fona_cmd(at,msg,sizeof(msg)) != 0) goto end;
  Serial.println(msg);
  at=(char*)"ATE0\n";
  if(fona_cmd(at,msg,sizeof(msg)) != 0) goto end;
  Serial.println(msg);
  at=(char*)"AT+COPS?\n";
  if(fona_cmd(at,msg,sizeof(msg)) != 0) goto end;
  Serial.println(msg);
  at=(char*)"AT+CBC\n";
  if(fona_cmd(at,msg,sizeof(msg)) != 0) goto end;
  Serial.println(msg);

  at=(char*)"AT+CPIN?\n";
  if(fona_cmd(at,msg,sizeof(msg)) != 0) goto end;
  Serial.println(msg);
  
  if(strcmp(msg,"+CPIN: SIM PIN") == 0) setpin=1;

  if(setpin == 1) {
Serial.println("pin processing begin");
    at=(char*)"AT+CPIN=3001\n";

    
//    if(fona_cmd(at,msg,sizeof(msg)) != 0) goto end;
//    Serial.println(msg);
    
    Serial1.write(at,strlen(at));
    len=0;
    for(;;) {
      if((ret=fona_read(msg,sizeof(msg)-1,&len)) == 1) {
        msg[len]='\0';
        if(*msg != '\0') {
          Serial.println(msg);
        }
        if(strcmp(msg,"OK") == 0) cpinok=1;
        else if(strcmp(msg,"+CPIN: READY") == 0) pinready=1;
        else if(strcmp(msg,"Call Ready") == 0) callready=1;
        else if(strcmp(msg,"SMS Ready") == 0) smsready=1;
        if((cpinok == 1) && (pinready == 1) && (callready == 1) && (smsready == 1)) break;
        len=0;
      } else if(ret != 0) goto end;
    }
Serial.println("pin processing end");
  }

  at=(char*)"AT+COPS?\n";
  if(fona_cmd(at,msg,sizeof(msg)) != 0) goto end;
  Serial.println(msg);
  
  at=(char*)"AT+CMGF=1\n";
  if(fona_cmd(at,msg,sizeof(msg)) != 0) goto end;
  Serial.println(msg);
  
  at=(char*)"AT+CMGL=\"ALL\"\n";
  if(fona_cmd(at,msg,sizeof(msg)) != 0) goto end;
  Serial.println(msg);
  
  at=(char*)"AT+CMGDA=\"DEL ALL\"\n";
  if(fona_cmd(at,msg,sizeof(msg)) != 0) goto end;
  Serial.println(msg);
  
  rval=0;

end:

  Serial.println("fona_init end");
  
  return(rval);
}

void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);

  Wire.begin();

  delay(1000);
  
  fona_init();
}

int smscmdproc(char *sms)
{
  static unsigned long xx=0;

  if(strlen(sms) > 20) {
    sms=sms+strlen(sms)-20;
  }

  xx++;
  Serial.print(xx);
  Serial.print("Sending msg ");
  Serial.println(sms);
  Wire.beginTransmission(17);
  Wire.write(sms);
//  Wire.write(sms+strlen(sms)-20);
  Wire.write(0);
  Wire.endTransmission(1);
  
  return(0);
}

void loop()
{
  int ret,smsi;
  char *at,msg[1024]={0},sms[32];
  size_t len;
  
  delay(1000);
//  Serial.println("loop");
//  smscmdproc("xxxxxxxxxx xxxxxxxxxx test i2c message");

  len=0;
  if((ret=fona_read(msg,sizeof(msg)-1,&len)) != 1) {
    msg[len]='\0';
    at=(char*)"AT+CMGL=\"ALL\"\n";
    if(fona_cmd(at,msg,sizeof(msg)) != 0) {
      return;
    }
    if(*msg == '\0') {
      Serial.println("no sms");
    } else {
      Serial.println(msg);
    }
    return;
  }
  
//  Serial.println(msg);

  at=(char*)"+CMTI: \"SM\",";
  if(strncmp(msg,at,strlen(at)) == 0) {
    smsi=atoi(msg+strlen(at));
    sprintf(sms,"AT+CMGR=%d\n",smsi);
    at=(char*)sms;
//Serial.print("get sms cmd: ");
//Serial.println(at);
    if(fona_cmd(at,msg,sizeof(msg)) == 0) {
      Serial.print("get sms data: ");
      Serial.println(msg);
      smscmdproc(msg);
    }
    sprintf(sms,"AT+CMGD=%d,0\n",smsi);
    at=(char*)sms;
//Serial.print("del sms cmd: ");
//Serial.println(at);
    if(fona_cmd(at,msg,sizeof(msg)) != 0) {
      return;
    }
    Serial.println(msg);
  } else {
    Serial.print("Message from fona808: ");
    Serial.println(msg);
  }

}
