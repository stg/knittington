#define pinPort( P ) ( ( P ) <= 7 ? &PORTD : ( ( P ) <= 13 ? &PORTB : &PORTC ) )
#define pinBit( P ) ( ( P ) <= 7 ? ( P ) : ( ( P ) <= 13 ? ( P ) - 8 : ( P ) - 14 ) )
#define fastWrite( P, V ) bitWrite( *pinPort( P ), pinBit( P ), ( V ) )
#define fastRead( P ) bitRead( *pinPort( P ), pinBit( P ) )

void setup() {
  pinMode(0, INPUT);
  pinMode(1, OUTPUT);
  pinMode(2, INPUT);
  pinMode(3, OUTPUT);
}

void loop() {
  // disable interrupts - reduces jitter
  cli();
  while(1) {
    // copy+invert data between pc usb port (0&1)
    // and machine "virtual" serial port (2&3)
    fastWrite(3,!fastRead(0));
    fastWrite(2,!fastRead(1));    
  }
}
