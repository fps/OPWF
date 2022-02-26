#include <TimerOne.h>
#include <EncoderButton.h>
#include <Button2.h>

#include <U8g2lib.h>

#pragma GCC diagnostic error "-Wall"

#define STOP_BUTTON 41

#define ENCODER_PUSH_BUTTON 35
#define ENCODER_BUTTON1 31 
#define ENCODER_BUTTON2 33

#define MOTOR_Z_ENABLE 38
#define MOTOR_Z_DIRECTION 55
#define MOTOR_Z_STEP 54

U8G2_ST7920_128X64_F_SW_SPI u8g(U8G2_R0, 23, 17, 16);

struct menu_item {
  const char* text;
  float value;
  float increment;
};

int current_menu_entry_index = 0;
bool menu_needs_redraw = false;
const byte number_of_menu_entries = 4;
menu_item menu[number_of_menu_entries] = {
  { "Target Winds  ",    1000, 100   },
  { "Current Winds ",       0,   0   },
  { "Speed W/s     ",       1,   0.1 },
  { "Accel. W/s^2  ",     0.1,   0.1 }
};

bool menu_data_entry_active = false;
int last_menu_data_entry_button_state = HIGH;

bool start_active = false;
int last_start_button_state = HIGH;
long start_time_usec = 0;

EncoderButton encoder(ENCODER_BUTTON1, ENCODER_BUTTON2);

const long timer_period_usec = 10L;

const long steps_per_turn = 200L;
const long microsteps = 16L;

long rounds_per_second = 1L;
long target_winds = 1000L;
long current_winds = 0L;
long acceleration = 1L;

void draw_menu() {
  u8g.firstPage();
  do {
    byte y_position = 7;
    const byte y_spacing = 8;
    u8g.setFont(u8g2_font_5x7_mr);
    for (byte menu_entry_index = 0; menu_entry_index < number_of_menu_entries; ++menu_entry_index) {  
      u8g.setDrawColor(2);
      if (menu_entry_index == current_menu_entry_index) {
        u8g.setDrawColor(0);    
      }
      u8g.setCursor(0, y_position);
      u8g.print(menu[menu_entry_index].text); u8g.print(menu[menu_entry_index].value);    
      y_position += y_spacing;      
    }
  } while ( u8g.nextPage() );  
}

void setup() {
  Serial.begin(115200);
  
  u8g.begin();
  draw_menu();

  // Beeper
  pinMode(37, OUTPUT);

  // Encoder buttons
  pinMode(ENCODER_PUSH_BUTTON, INPUT_PULLUP);
  pinMode(ENCODER_BUTTON1, INPUT_PULLUP);
  pinMode(ENCODER_BUTTON2, INPUT_PULLUP);

  pinMode(STOP_BUTTON, INPUT_PULLUP);

  pinMode(38, OUTPUT);
  pinMode(55, OUTPUT);
  pinMode(54, OUTPUT);

  digitalWrite(38, LOW);
  digitalWrite(55, LOW);

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
  if (start_active && elapsed_since_step_usec > step_period_usec) {
    if (steps_taken >= max_steps) return;
    
    elapsed_since_step_usec -= step_period_usec;

    PORTF |= B00000001;
    PORTF &= B11111110;
  
    ++steps_taken;
  }

  elapsed_since_step_usec += timer_period_usec;
}

long last_output_usec = 0;
const long output_period_usec = 1e5L;

int last_encoder_position = 0;
long last_encoder_change_usec = 0;

void loop() {
  const long now_usec = micros();
  encoder.update();

  steps_per_second = microsteps * (10L + ((now_usec - start_time_usec) / 10000L));
  // steps_per_second = 5L;
  if (steps_per_second > steps_per_second_limit) {
    steps_per_second = steps_per_second_limit;
  }
  
  step_period_usec = 1e6L / steps_per_second;  

  int push_button = digitalRead(ENCODER_PUSH_BUTTON);
  if (push_button != last_menu_data_entry_button_state) {
    if (push_button == LOW) {
      menu_data_entry_active = !menu_data_entry_active;
    }
    last_menu_data_entry_button_state = push_button;
    delay(100);
  }

  int stop_button = digitalRead(STOP_BUTTON);   
  if (stop_button != last_start_button_state) {
    if (stop_button == LOW) {
      start_active = !start_active;
      if (start_active) {
        start_time_usec = now_usec;
        steps_taken = 0;
        //last_step_usec = now_usec;
        digitalWrite(MOTOR_Z_ENABLE, LOW);
      } else {
        digitalWrite(MOTOR_Z_ENABLE, HIGH);
      }
    }
    last_start_button_state = stop_button;
    delay(100);
  }

  const int encoder_position = encoder.position();
  if (encoder_position != last_encoder_position) {
    if (menu_data_entry_active) {
      int delta = encoder_position - last_encoder_position;
      menu[current_menu_entry_index].value += menu[current_menu_entry_index].increment * delta;
    } else {
      current_menu_entry_index += encoder_position - last_encoder_position;
      if (current_menu_entry_index < 0) current_menu_entry_index = 0;
      if (current_menu_entry_index > (number_of_menu_entries - 1)) current_menu_entry_index = (number_of_menu_entries - 1);
    }
    last_encoder_change_usec = now_usec;
    menu_needs_redraw = true;
    last_encoder_position = encoder_position;
  }
  
  if (menu_needs_redraw && now_usec - last_encoder_change_usec > 5e5L)
  {
    draw_menu();
    menu_needs_redraw = false;
  }
    
  if (now_usec - last_output_usec > output_period_usec) {
    last_output_usec = now_usec;

    Serial.print(encoder.position());
    Serial.print(" ");
    Serial.print(digitalRead(ENCODER_PUSH_BUTTON));
    Serial.print(" ");
    Serial.print(digitalRead(STOP_BUTTON));
    Serial.print(" ");
    Serial.println(steps_taken / (steps_per_turn * microsteps));
  }  

  return;
}
