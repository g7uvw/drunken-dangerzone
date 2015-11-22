void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600); 
  

 
}

void loop()
{
 
 Serial.println(1);
 delay(2000);
 Serial.print(150);
 delay(2000);
}
