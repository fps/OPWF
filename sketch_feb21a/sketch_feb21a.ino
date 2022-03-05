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

Button2 encoder_push_button;
Button2 stop_button;

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
  { "Current Winds ",       0, 100   },
  { "Speed W/s     ",       1,   0.1 },
  { "Accel. W/s^2  ",     0.1,   0.1 }
};

bool menu_data_entry_active = false;

volatile bool start_active = false;

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
      if (menu_entry_index == current_menu_entry_index && !menu_data_entry_active) {
        u8g.setDrawColor(0);    
      }
      u8g.setCursor(0, y_position);
      u8g.print(menu[menu_entry_index].text); 
      
      if (menu_entry_index == current_menu_entry_index && menu_data_entry_active) {
        u8g.setDrawColor(0);    
      }
      u8g.print(menu[menu_entry_index].value);    
      y_position += y_spacing;      
    }
  } while ( u8g.nextPage() );  
}

void setup() {
  Serial.begin(115200);

  stop_button.begin(STOP_BUTTON);
  stop_button.setClickHandler(button_handler);
  stop_button.setDoubleClickHandler(button_handler);
  encoder_push_button.begin(ENCODER_PUSH_BUTTON);
  encoder_push_button.setClickHandler(button_handler);
  
  u8g.begin();
  draw_menu();

  // Beeper
  pinMode(37, OUTPUT);

  // Encoder buttons
  pinMode(ENCODER_PUSH_BUTTON, INPUT_PULLUP);
  pinMode(ENCODER_BUTTON1, INPUT_PULLUP);
  pinMode(ENCODER_BUTTON2, INPUT_PULLUP);

  pinMode(STOP_BUTTON, INPUT_PULLUP);

  pinMode(MOTOR_Z_ENABLE, OUTPUT);
  pinMode(MOTOR_Z_DIRECTION, OUTPUT);
  pinMode(MOTOR_Z_STEP, OUTPUT);

  digitalWrite(MOTOR_Z_ENABLE, HIGH);
  digitalWrite(MOTOR_Z_DIRECTION, HIGH);

  Timer1.initialize();
  Timer1.attachInterrupt(process, timer_period_usec);
}
  
volatile long step_period_usec = 1e6L;

unsigned long steps_per_second;
unsigned long elapsed_usec = 0;

long steps_per_second_limit = 1L;

volatile long max_steps = 1L;

volatile long steps_taken = 0;

long elapsed_since_step_usec = 0;
void process() {
  if (start_active && elapsed_since_step_usec > step_period_usec) {
    if (steps_taken < max_steps) {    
      elapsed_since_step_usec -= step_period_usec;
  
      PORTF |= B00000001;
      PORTF &= B11111110;
    
      ++steps_taken;
    }
  }

  elapsed_since_step_usec += timer_period_usec;
}

unsigned long last_output_usec = 0;
const unsigned long output_period_usec = 5e5L;

int last_encoder_position = 0;
unsigned long last_encoder_change_usec = 0;

volatile unsigned long now_usec = 0;

void button_handler(Button2 &button) {
  if (button == stop_button) {
      if (button.getClickType() == SINGLE_CLICK) {
        start_time_usec = now_usec;
        steps_per_second_limit = menu[2].value * steps_per_turn * microsteps;
        max_steps = menu[0].value * steps_per_turn * microsteps;
        step_period_usec = 1e6L;
        elapsed_since_step_usec = 0;

        Serial.print("limit: "); Serial.println(steps_per_second_limit); 
        
        if (!start_active) {
          digitalWrite(MOTOR_Z_ENABLE, LOW);
        } else {
          digitalWrite(MOTOR_Z_ENABLE, HIGH);
        }
        
        start_active = !start_active;

        if (!start_active) {
          menu[1].value = (float)steps_taken / (steps_per_turn * microsteps);
          draw_menu();
        }
      }

      if (button.getClickType() == DOUBLE_CLICK) {
        menu[1].value = 0;
        steps_taken = 0;
        draw_menu();
      }
  }

  if (button == encoder_push_button) {
    menu_data_entry_active = !menu_data_entry_active;
    draw_menu();
  }
}

void loop() {
  now_usec = micros();
  encoder.update();
  stop_button.loop();
  encoder_push_button.loop();

  if (start_active) {
    steps_per_second = microsteps * (10L + ((now_usec - start_time_usec) / 10000L));
    // steps_per_second = 5L;
    if (steps_per_second > steps_per_second_limit) {
      steps_per_second = steps_per_second_limit;
    }
    
    step_period_usec = 1e6L / steps_per_second;  
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
    
  if (true && now_usec - last_output_usec > output_period_usec) {
    last_output_usec = now_usec;

    Serial.print(" enc pos: ");
    Serial.print(encoder.position());
    Serial.print(" enc push: ");
    Serial.print(digitalRead(ENCODER_PUSH_BUTTON));
    Serial.print(" stop: ");
    Serial.print(digitalRead(STOP_BUTTON));
    Serial.print(" winds: ");
    Serial.print((float)steps_taken / (steps_per_turn * microsteps));
    Serial.print(" steps: ");
    Serial.print(steps_taken);
    Serial.print(" sps: ");
    Serial.print(steps_per_second);
    Serial.print(" step period: ");
    Serial.print(step_period_usec);
    Serial.println("");
  }  

  return;
}
