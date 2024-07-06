#include <EEPROM.h>
#define CHARGE_PIN A0
#define WAKEUP_PIN A1
#define POWER_BANK_CONNECTED 1u
#define POWER_BANK_DISCONNECTED 0u

#define CHARGING_THRESHOLD_VALUE 80           //60//       //approximately 3.5 days
#define DISCHARGING_THRESHOLD_VALUE 6         //30//       // 6 hours
#define WAKEUP_THRESHOLD_VALUE_DISCONNECT 15  //15 SEC
#define WAKEUP_THRESHOLD_VALUE_CONNECT 1      //1 SEC
#define STOREDATA_THRESHOLD_VALUE 3           //10//       // 3 hours
/*****************Global Variables*****************************/
unsigned char ChargingClock = 0;
unsigned char DischargingClock = 0;
unsigned char WakeUpClock = 0;
unsigned char storeData = 0;
unsigned char LockFlag = 0;
unsigned long int SEC_counter = 0;
/*****************Functions Prototypes*************************/
void Timer0_configuration();
unsigned char EEPROM_read(unsigned int uiAddress);
void EEPROM_write(unsigned int uiAddress, unsigned char ucData);
unsigned long int EEPROM_initRead(unsigned int uiAddress);

// the setup routine runs once when you press reset:
void setup() {
  // initialize the digital pin as an output.
  Serial.begin(9600);
  pinMode(CHARGE_PIN, OUTPUT);
  pinMode(WAKEUP_PIN, OUTPUT);
  pinMode(7, OUTPUT);
  digitalWrite(CHARGE_PIN, LOW);
  digitalWrite(WAKEUP_PIN, LOW);
  digitalWrite(7, HIGH);
  Timer0_configuration();
  //for(int i=0 ; i<10 ; i++){EEPROM[i] = 0xff ;}
  SREG &= 0x7f;  //disable global interrupt.
  /*Begin of critical section*/
  if (EEPROM.read(0) == 13) {
    ChargingClock = EEPROM.read(1);     // read from address 1 ,2 ,3 ,4
    DischargingClock = EEPROM.read(2);  // read from address 5 ,6 ,7 ,8
    LockFlag = EEPROM.read(3);
    if (LockFlag == 255) LockFlag = 0;
  }
  /*End of critical section*/
  SREG |= 0x80;  //enable global interrupt.
}

// the loop routine runs over and over again forever:
void loop() {
  switch (LockFlag) {
    case POWER_BANK_DISCONNECTED:
      if (ChargingClock == CHARGING_THRESHOLD_VALUE) {
        digitalWrite(CHARGE_PIN, HIGH);  //charge the power bank
        LockFlag = POWER_BANK_CONNECTED;
        ChargingClock = 0;
        EEPROM[1] = ChargingClock;
        EEPROM[3] = LockFlag;
      }
      else if (ChargingClock < CHARGING_THRESHOLD_VALUE) {
        digitalWrite(CHARGE_PIN, LOW);  //disconnect the charger from the power bank
      }
      if (WakeUpClock == WAKEUP_THRESHOLD_VALUE_DISCONNECT) {
        digitalWrite(WAKEUP_PIN, HIGH);  //connect a load temporarely to the power bank so that the power bank has to wake up...
        WakeUpClock = 0;
      } else if (WakeUpClock >= WAKEUP_THRESHOLD_VALUE_CONNECT) {
        digitalWrite(WAKEUP_PIN, LOW);  //connect a load temporarely to the power bank so that the power bank has to wake up...
      }
      break;
    case POWER_BANK_CONNECTED:
      if (DischargingClock == DISCHARGING_THRESHOLD_VALUE) {
        digitalWrite(CHARGE_PIN, LOW);  //disconnect the charger from the power bank
        LockFlag = POWER_BANK_DISCONNECTED;
        DischargingClock = 0;
        EEPROM[2] = DischargingClock;
        EEPROM[3] = LockFlag;
      }
      if (DischargingClock < DISCHARGING_THRESHOLD_VALUE) {
        digitalWrite(CHARGE_PIN, HIGH);  //charge the power bank
      }
      break;
  }
  if (storeData == STOREDATA_THRESHOLD_VALUE) {
    SREG &= 0x7f;  //disable global interrupt.
    /*Begin of critical section*/
    if (EEPROM.read(0) != 13) {
      EEPROM.update(0, 13);
    }
    EEPROM.update(1, ChargingClock);
    EEPROM.update(2, DischargingClock);
    EEPROM.update(3, LockFlag);
    storeData = 0;
    /*End of critical section*/
    SREG |= 0x80;  //enable global interrupt.
  }
}

void Timer0_configuration() {
  TCCR0A = 0x02;  //CTC mode is activated..
  OCR0A = 250;    //Ref value = 250 ... so that 16uSec * 250 = 4mSec
  TCCR0B = 0x04;  //Prescaler is CLKio/256... Frq..T0 = 62500 HZ
  TIMSK0 = 0x02;  //enable interrupt of CTC channel A.
}

void EEPROM_write(unsigned int uiAddress, unsigned char ucData) {
  /* Wait for completion of previous write */
  while (EECR & (1 << EEPE))
    ;
  /* Set up address and Data Registers */
  EEAR = uiAddress;
  EEDR = ucData;
  /* Write logical one to EEMPE */
  EECR |= (1 << EEMPE);
  /* Start eeprom write by setting EEPE */
  EECR |= (1 << EEPE);
}

unsigned char EEPROM_read(unsigned int uiAddress) {
  /* Wait for completion of previous write */
  while (EECR & (1 << EEPE))
    ;
  /* Set up address register */
  EEAR = uiAddress;
  /* Start eeprom read by writing EERE */
  EECR |= (1 << EERE);
  /* Return data from Data Register */
  return EEDR;
}

unsigned long int EEPROM_initRead(unsigned int uiAddress) {
  unsigned long int container = 0;
  for (int i = 0; i < 4; i++) {
    char temp_data = EEPROM_read(i + uiAddress);
    container |= ((unsigned long int)temp_data << (i * 8));
  }
  return container;
}

ISR(TIMER0_COMPA_vect) {
  //interrupt ISR invoked every 4 mSec ...
  SEC_counter++;
  if (SEC_counter % 250 == 0) {
    //to increment every 1 Sec ...
    if (WakeUpClock != WAKEUP_THRESHOLD_VALUE_DISCONNECT) WakeUpClock++;
    Serial.println(EEPROM.read(0));
    Serial.print("chargeClock : ");
    Serial.println(ChargingClock);
    Serial.print("LOCK : ");
    Serial.println(LockFlag);
    Serial.print("DischargeClock : ");
    Serial.println(DischargingClock);
    Serial.print("EEPROM[1] : ");
    Serial.println(EEPROM[1]);
    Serial.print("EEPROM[2] : ");
    Serial.println(EEPROM[2]);
  }
  if (SEC_counter == 900000) {  //to increment every 1 hour ... 7500
    SEC_counter = 0;
    if (storeData != STOREDATA_THRESHOLD_VALUE) storeData++;
    if (ChargingClock < CHARGING_THRESHOLD_VALUE && LockFlag == POWER_BANK_DISCONNECTED) ChargingClock++;
    if (DischargingClock < DISCHARGING_THRESHOLD_VALUE && LockFlag == POWER_BANK_CONNECTED) DischargingClock++;
  }
}