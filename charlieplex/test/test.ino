
unsigned int a,f;


//unsigned char LEDADDR[]  = {3,6,12,24,48,05,10,40,18,17,33,20,36};
unsigned char LEDADDR[]  = {3,6,12,24,48,05,10,40,18,20,33,24,17,36,18,40};
unsigned char LEFT[] = {3,6,12,24,36,17,18,40};
unsigned char RIGHT[] = {40,18,20,10,5,33,48,24};

unsigned char fcount = 32;

//unsigned char FRAMES[] = { 2,1,
//                           2,4,
//                           8,4,
//                           16,8,
//                           16,32,
//                           4,1,
//                           2,8,
//                           32,8,
//                           2,16,
//                           16,1,
//                           1,32,
//                           16,4,
//                           32,4};
                           
unsigned char FRAMES[] = { 2,1,
                           2,4,
                           8,4,
                           16,0,
                           16,32,
                           4,1,
                           2,8,
                           32,0,
                           2,16,
                           4,16,
                           32,1,
                           8,0,
                           1,16,
                           4,32,    
                           16,2,
                           8,0
                           };
                           
unsigned char LEFTFRAME[] = { 2,1,
                              2,4,
                              8,4,
                              16,8,
                              32,4,
                              16,1,
                              16,2,
                              8,0};
                              
unsigned char RIGHTFRAME[] = { 22,0,
                              16,2,
                              16,4,
                              2,8,
                              4,1,
                              1,32,
                              16,32,
                              16,8};

void setup()
{
  DDRB = B00000000;   //all inputs (oposite of pic)
 // Serial.begin(9600); 

  // prints title with ending line break 
  //Serial.println("Charlieplex test"); 
  
}

void trace()
{
  int ff=0,loopcount=500;
  while (loopcount >0)
  {
    for (a=0; a<16; a++)
    {
       DDRB = LEDADDR[a];
  
       for (f=0;f < 2; f++)
        {
          if (FRAMES[ff] != 0)
            {
              PORTB = FRAMES[ff];
              delay(loopcount);
             }
          ff++;
         }

        if (ff == fcount) ff=0;
  
      }
    loopcount -=100;
  }
  
}

void allon(int count)
{
  
  //all on
int ff=0;
while (count >0)
  {
for (a=0; a<16; a++)
  {
     DDRB = LEDADDR[a];
  
    for (f=0;f < 2; f++)
    {
      if (FRAMES[ff] != 0)
      {
        PORTB = FRAMES[ff];
          delayMicroseconds(50);        // pauses for 50 microseconds      
       }
      ff++;
     }

  if (ff == fcount) ff=0;
  
  }
  count--;
  }
}

void left(int count)
{
    
int ff=0;
while (count >0)
  {
  for (a=0; a<8; a++)
    {
     DDRB = LEFT[a];
  
      for (f=0;f < 2; f++)
      {
        if (LEFTFRAME[ff] != 0)
          {
          PORTB = LEFTFRAME[ff];
            delayMicroseconds(50);        // pauses for 50 microseconds      
           }
        ff++;
       }

  if (ff == 16) ff=0;
  }
  count--;
  }
}
  
void right(int count)
{
    
int ff=0;
while (count >0)
  {
  for (a=0; a<8; a++)
    {
     DDRB = RIGHT[a];
  
      for (f=0;f < 2; f++)
      {
        if (RIGHTFRAME[ff] != 0)
          {
          PORTB = RIGHTFRAME[ff];
           delayMicroseconds(50);        // pauses for 50 microseconds      
           }
        ff++;
       }

  if (ff == 16) ff=0;
  }
  count--;
  }
}

void alloff(int count)
{
  PORTB=0;
  delay(count);
}

void dash()
{
  allon(1000);
  alloff(300);
}

void dot()
{
  allon(333);
  alloff(300);
}
void space()
{
  alloff(700);
}
  

void morse()
{
 // -.. .- ...- .. -.. / .-.. --- ...- . ... / -- . .- --. . -. 
dash();
dot();
dot();
space();
dot();
dash();
space();
dot();
dot();
dot();
dash();
space();
dot();
dot();
space();
dash();
dot();
dot();
space();
space();
dot();
dash();
dot();
dot();
space();
dash();
dash();
dash();
space();
dot();
dot();
dot();
dash();
space();
dot();
space();
dot();
dot();
dot();
space();
space();
dash();
dash();
space();
dot();
space();
dot();
dash();
space();
dash(); 
dash(); 
dot();
dot();
space();
dash();
dot();

  
}

void loop()
{
  
 trace();
  allon(1000);
  left(1000);
  right(1000);
  left(500);
  right(500);
  left(250);
  right(250);
  allon(500);
  PORTB=0;
  morse();
  delay(1000);
}
