#include <TimerOne.h>
#include <EncoderButton.h>
#include <U8g2lib.h>

#pragma GCC diagnostic error "-Wall"

U8G2_ST7920_128X64_1_SW_SPI u8g(U8G2_R0, 23, 17, 16);

struct menu_item {
  const char* text;
  float value;
  float increment;
};

byte current_menu_entry_index = 0;
const byte number_of_menu_entries = 4;
menu_item menu[number_of_menu_entries] = {
  { "Target Winds     ",    1000, 100   },
  { "Current Winds    ",       0,   0   },
  { "Winds per Second ",       0,   0.1 },
  { "Acceleration     ",       1,   0.1 }
};

EncoderButton encoder(33, 31, 35);

const long timer_period_usec = 10L;

const long steps_per_turn = 200L;
const long microsteps = 16L;

long rounds_per_second = 1L;
long target_winds = 1000L;
long current_winds = 0L;
long acceleration = 1L;

void setup() {
  Serial.begin(115200);
  
  u8g.begin();

  u8g.firstPage();
  do {
    byte y_position = 7;
    const byte y_spacing = 8;
    u8g.setFont(u8g2_font_5x7_mr);
    u8g.setDrawColor(2);
    for (byte menu_entry_index = 0; menu_entry_index < number_of_menu_entries; ++menu_entry_index) {  
      u8g.setCursor(0, y_position);
      u8g.print(menu[menu_entry_index].text); u8g.print(menu[menu_entry_index].value);    
      y_position += y_spacing;      
    }
  } while ( u8g.nextPage() );

  // Beeper
  pinMode(37, OUTPUT);

  // Encoder buttons
  pinMode(35, INPUT_PULLUP);
  pinMode(33, INPUT_PULLUP);
  pinMode(31, INPUT_PULLUP);
  
  pinMode(38, OUTPUT);
  pinMode(56, OUTPUT);
  pinMode(62, OUTPUT);
  
  pinMode(55, OUTPUT);
  pinMode(61, OUTPUT);
  pinMode(48, OUTPUT);
  
  pinMode(54, OUTPUT);
  pinMode(60, OUTPUT);
  pinMode(46, OUTPUT);

  digitalWrite(38, LOW);
  digitalWrite(56, LOW);
  digitalWrite(62, LOW);

  digitalWrite(55, HIGH);
  digitalWrite(61, HIGH);  
  digitalWrite(48, HIGH);

  Timer1.initialize();
  Timer1.attachInterrupt(process, timer_period_usec);
}
  
long step_period_usec = 1e6L;

long steps_per_second;
long elapsed_usec = 0;
long elapsed_control_periods = 0;

const long steps_per_second_limit = 2L * steps_per_turn * microsteps;

const long max_steps = 100L * steps_per_turn * microsteps;

volatile long steps_taken = 0;

long elapsed_since_step_usec = 0;
void process() {
  if (elapsed_since_step_usec > step_period_usec) {
    if (steps_taken >= max_steps) return;
    
    elapsed_since_step_usec -= step_period_usec;

    // digitalWrite(54, HIGH);
    PORTF |= B00000001;
    // delayMicroseconds(1);
    PORTF &= B11111110;
  
    // digitalWrite(54, LOW);
    ++steps_taken;
  }

  elapsed_since_step_usec += timer_period_usec;
}

long last_output_usec = 0;
const long output_period_usec = 1e5L;
const long main_loop_delay_usec = 1e2L;

void handle_serial_input() {
  
}

void loop() {

  handle_serial_input();
  
  const long now_usec = micros();
  encoder.update();


  
  steps_per_second = microsteps * (10L + (now_usec / 10000L));
  // steps_per_second = 5L;
  if (steps_per_second > steps_per_second_limit) {
    steps_per_second = steps_per_second_limit;
  }
  
  step_period_usec = 1e6L / steps_per_second;  

  if (now_usec - last_output_usec > output_period_usec) {
    last_output_usec = now_usec;
    
    Serial.print(encoder.position());
    Serial.print(" ");
    Serial.print(digitalRead(35));
    Serial.print(" ");
    Serial.println(steps_taken / (steps_per_turn * microsteps));
  }  
    
  return;
}
