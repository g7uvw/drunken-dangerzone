/*
  String substring()

  Examples of how to use substring in a String

  created 27 Jul 2010,
  modified 2 Apr 2012
  by Zach Eveland

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/StringSubstring
*/

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // send an intro:
  Serial.println("\n\nString  substring():");
  Serial.println();
}

void loop() {
  // Set up a String:
  String _date = "12-34-5678";
  String _time = "12:34:56";
  Serial.println(_date);
  Serial.println(_time);
  
  Serial.println(_date.substring(6,10));
  Serial.println(_date.substring(3,5));
  Serial.println(_date.substring(0,2));

  Serial.println(_time.substring(0,2));
  Serial.println(_time.substring(3,5));
  Serial.println(_time.substring(6,8));
  
  
  
  //char buff[10];
  //sprintf(buff,"Year = %ld", (_date.substring(6,10).toInt()));
  //Serial.println(buff);
  
  
  
  
  /*
  sprintf(buff,"Month = %ld", ((_date.substring(3,5).toInt())));
  Serial.println(buff);
  sprintf(buff,"Day = %ld", ((_date.substring(0,1).toInt())));
  Serial.println(buff);
  sprintf(buff,"Hour = %ld", ((_time.substring(0,1).toInt())));
  Serial.println(buff);
  sprintf(buff,"Min = %ld", ((_time.substring(3,4).toInt())));
  Serial.println(buff);
  sprintf(buff,"Sec = %ld", ((_time.substring(6,7).toInt())));
  Serial.println(buff);
  
*/

  // do nothing while true:
  while (true);
}
