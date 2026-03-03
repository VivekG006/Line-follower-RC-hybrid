   #define IR1 34
   #define IR2 35
   #define IR3 32
   #define IR4 33
   #define IR5 25
   #define BLUE 2
void setup() {
  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT); 
  pinMode(IR3, INPUT);
  pinMode(IR4, INPUT);
  pinMode(IR5, INPUT);
  pinMode(BLUE, OUTPUT);


}

void loop() {
  int var1 = digitalRead(IR1);
  int var2 = digitalRead(IR2);
  int var3 = digitalRead(IR3);
  int var4 = digitalRead(IR4);
  int var5 = digitalRead(IR5);
  int weight = 0;
  if(var1 = 0){
    weight = -2;

    
  }

}
