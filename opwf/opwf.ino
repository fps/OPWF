#define ENCODER_USE_INTERRUPTS
#include <EncoderButton.h>

#include <Button2.h>
#include <Servo_Hardware_PWM.h>
#include <EEPROM.h>
#include <U8g2lib.h>

// #################### CONSTANTS

#define STOP_BUTTON 41

#define ENCODER_PUSH_BUTTON 35
#define ENCODER_BUTTON1 31 
#define ENCODER_BUTTON2 33

#define MOTOR_Z_ENABLE 38
#define MOTOR_Z_DIRECTION 55
#define MOTOR_Z_STEP 54

#define SERVO_PIN 5

const long timer_period_usec = 20L;
const long steps_per_turn = 200L;
const long microsteps = 16L;


// #################### GLOBAL STATE

long step_period_usec = 0;
unsigned long steps_per_second;
long elapsed_since_step_usec = 0L;


unsigned long steps_per_second_limit = 1L;
volatile long max_steps = 1L;
volatile long steps_taken = 0L;

// volatile bool start_active = false;
long start_time_usec = 0;


// #################### SERVO

Servo servo;


// #################### BUTTONS

Button2 encoder_push_button;
Button2 stop_button;

volatile enum {
  STOPPED = 0,
  STARTING,
  RUNNING,
  STOPPING
} state;


// #################### MENU

U8G2_ST7920_128X64_F_SW_SPI u8g(U8G2_R0, 23, 17, 16);

struct menu_item {
  const char* text;
  float value;
  float increment;
  float minimum;
  float maximum;
};

enum menu_entry_indices {
  TARGET_WINDS = 0,
  CURRENT_WINDS,
  WIND_DIRECTION,
  SPEED,
  ACCELERATION,
  WINDS_PER_SWEEP,
  RIGHT_LIMIT,
  LEFT_LIMIT,
  NUMBER_OF_MENU_ENTRIES
};

int current_menu_entry_index = 0;
bool menu_needs_redraw = false;

menu_item menu[NUMBER_OF_MENU_ENTRIES] = {
  { "Target Winds     ",    1000,  50,     0, 20000 },
  { "Current Winds    ",       0,   0,     0, 20000 },
  { "Wind Direction   ",       1,   2,    -1,     1 },
  { "Speed W/s        ",       2,   0.1, 0.1,    10 },
  { "Accel. W/s^2     ",     1.0,   0.1, 0.1,    10 },
  { "Winds/Sweep      ",     100,  10,     0,  5000 },
  { "Right limit deg. ",      90,   1,     0,   180 },
  { "Left Limit deg.  ",     120,   1,     0,   180 }
};

bool menu_data_entry_active = false;

EncoderButton encoder(ENCODER_BUTTON1, ENCODER_BUTTON2);

void draw_menu() {
  u8g.firstPage();
  do {
    byte y_position = 7;
    const byte y_spacing = 8;
    u8g.setFont(u8g2_font_5x7_mr);
    for (byte menu_entry_index = 0; menu_entry_index < NUMBER_OF_MENU_ENTRIES; ++menu_entry_index) {  
      u8g.setDrawColor(2);
      if (menu_entry_index == current_menu_entry_index && !menu_data_entry_active) {
        u8g.setDrawColor(0);    
      }
      u8g.setCursor(0, y_position);
      u8g.print(menu[menu_entry_index].text); 
      
      if (menu_entry_index == current_menu_entry_index && menu_data_entry_active) {
        u8g.setDrawColor(0);    
      }
      char buffer[10];
      dtostrf(menu[menu_entry_index].value, 8, 2, buffer);
      u8g.print(buffer);    
      y_position += y_spacing;      
    }
  } while ( u8g.nextPage() );  
}


// #################### EEPROM

const float magic_cookie = 331.337f;

void read_eeprom() {
  Serial.println("Restoring from EEPROM...");
  for (unsigned index = 0; index < NUMBER_OF_MENU_ENTRIES; ++index) {
    EEPROM.get((index+1)*sizeof(float), menu[index].value);
  }
  Serial.println("Done.");
}

void save_eeprom() {
  Serial.println("Saving to EEPROM");
  EEPROM.put(0, magic_cookie);
  float eeprom_value;
  for (unsigned index = 0; index < NUMBER_OF_MENU_ENTRIES; ++index) {
    EEPROM.get((index+1)*sizeof(float), eeprom_value);
    if (eeprom_value != menu[index].value) {
      Serial.println(menu[index].value);
      EEPROM.put((index+1)*sizeof(float), menu[index].value);      
    }
  }
  Serial.println("Done.");
}


// #################### SETUP

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println("Open Winder Firmware v0.1 starting up...");

  Serial.println("Setting up EEPROM...");
  
  float eeprom_value;
  if (EEPROM.get(0, eeprom_value) == magic_cookie) {
    read_eeprom();
  } else {
    save_eeprom(); 
  }

  Serial.println("Setting up pins...");
  
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

  Serial.println("Setting up buttons...");
  
  stop_button.begin(STOP_BUTTON);
  stop_button.setClickHandler(button_handler);
  stop_button.setDoubleClickHandler(button_handler);
  encoder_push_button.begin(ENCODER_PUSH_BUTTON);
  encoder_push_button.setClickHandler(button_handler);


  Serial.println("Drawing menu...");
  
  u8g.begin();
  draw_menu();

  Serial.println("Setting up servo...");
  
  // Servo
  servo.attach(SERVO_PIN);
  servo.write(menu[LEFT_LIMIT].value);
  // pinMode(SERVO_PIN, OUTPUT);
  // analogWrite(SERVO_PIN, 128);

  // Beeper

  Serial.println("Setting up state...");
  state = STOPPED;
  
  Serial.println("Setting up timers...");
  /*
  Timer1.initialize();
  Timer1.attachInterrupt(process, timer_period_usec);
  */

  /* 
  FrequencyTimer2::setPeriod(timer_period_usec);
  FrequencyTimer2::setOnOverflow(process);
  */
  cli();
  //set timer2 interrupt at 8kHz
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 8khz increments
  // OCR2A = 249;// = (16*10^6) / (8000*8) - 1 (must be <256)
  OCR2A = 2 * timer_period_usec - 1;
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS21 bit for 8 prescaler
  TCCR2B |= (1 << CS21);   
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);
  sei();
  
  Serial.println("Startup done.");
}


// #################### INTERRUPTS

// void process() {
ISR(TIMER2_COMPA_vect) {
  cli();
  if (state != STOPPED) {
    if (steps_taken < max_steps) {
      if (elapsed_since_step_usec > step_period_usec) {
        elapsed_since_step_usec -= step_period_usec;
    
        PORTF |= B00000001;
        PORTF &= B11111110;
      
        ++steps_taken;
      }
    } else {
      state = STOPPING;
    }
  }
  elapsed_since_step_usec += timer_period_usec;
  sei();
}

// #################### BUTTON HANDLING

unsigned long last_output_usec = 0;
const unsigned long output_period_usec = 1e6L;

int last_encoder_position = 0;
unsigned long last_encoder_change_usec = 0;

volatile unsigned long now_usec = 0;

void button_handler(Button2 &button) {
  if (button == stop_button) {
      if (button.getClickType() == SINGLE_CLICK) {
        save_eeprom();
        start_time_usec = now_usec;
        steps_per_second_limit = menu[SPEED].value * steps_per_turn * microsteps;
        max_steps = menu[TARGET_WINDS].value * steps_per_turn * microsteps;
        step_period_usec = 1e6L;
        elapsed_since_step_usec = 0;

        Serial.print("limit: "); Serial.println(steps_per_second_limit); 

        if (state == STOPPED) {
          Serial.println("STOPPED -> STARTING...");
          digitalWrite(MOTOR_Z_ENABLE, LOW);
          if (menu[WIND_DIRECTION].value > 0) digitalWrite(MOTOR_Z_DIRECTION, HIGH); else digitalWrite(MOTOR_Z_DIRECTION, LOW);
          state = STARTING;
          return;
        }

        if (state == RUNNING) {
          Serial.println("RUNNING -> STOPPING...");
          state = STOPPING;
          return;
        }

        if (state == STARTING) {
          Serial.println("STARTING -> STOPPING...");
          state = STOPPING;
          return;
        }
      }

      if (button.getClickType() == DOUBLE_CLICK) {
        menu[CURRENT_WINDS].value = 0;
        steps_taken = 0;
        draw_menu();
        return;
      }
  }

  if (button == encoder_push_button) {
    menu_data_entry_active = !menu_data_entry_active;
    draw_menu();
  }
}


// #################### MAIN LOOP

void loop() {
  now_usec = micros();
  encoder.update();
  stop_button.loop();
  encoder_push_button.loop();
  
  // STEPPER CONTROL

  if (state == STOPPING) {
      Serial.println("STOPPING -> STOPPED...");
      digitalWrite(MOTOR_Z_ENABLE, HIGH);
      state = STOPPED;
      menu[CURRENT_WINDS].value = (float)steps_taken / (steps_per_turn * microsteps);
      draw_menu();
      Serial.println("Done.");
  }
  
  if (state == STARTING) {
    const float seconds_since_start = (now_usec - start_time_usec) / 1e6f;
    float winds_per_second = menu[ACCELERATION].value * seconds_since_start;
    if (winds_per_second > menu[SPEED].value) winds_per_second = menu[SPEED].value;
    
    // steps_per_second = microsteps * (10L + ((now_usec - start_time_usec) / 10000L));
    steps_per_second = 10L + winds_per_second * microsteps * steps_per_turn;
    // steps_per_second = 5L;
    if (steps_per_second > steps_per_second_limit) {
      steps_per_second = steps_per_second_limit;
      Serial.println("STARTING -> RUNNING");
      state = RUNNING;
    }
  }

  if (state != STOPPED) {
    step_period_usec = 1e6L / steps_per_second;

    const float current_wind = (float)steps_taken / (float)(steps_per_turn * microsteps);
    const float sweep_phase = fmodf(current_wind / menu[WINDS_PER_SWEEP].value, 1.0);
    // Serial.println(sweep_phase);
    if (sweep_phase < 0.5f) {
      const float half_phase = 2.0f * sweep_phase;
      servo.write((menu[LEFT_LIMIT].value - menu[RIGHT_LIMIT].value) * half_phase + menu[RIGHT_LIMIT].value);
    } else {
      const float half_phase = 2.0f * (sweep_phase - 0.5f);
      servo.write((menu[RIGHT_LIMIT].value - menu[LEFT_LIMIT].value) * half_phase + menu[LEFT_LIMIT].value);
    }
  }

  
  // MENU
  
  const int encoder_position = encoder.position();
  if (encoder_position != last_encoder_position) {
    if (menu_data_entry_active) {
      int delta = encoder_position - last_encoder_position;
      menu[current_menu_entry_index].value += menu[current_menu_entry_index].increment * delta;
      if (menu[current_menu_entry_index].value > menu[current_menu_entry_index].maximum) menu[current_menu_entry_index].value = menu[current_menu_entry_index].maximum;
      if (menu[current_menu_entry_index].value < menu[current_menu_entry_index].minimum) menu[current_menu_entry_index].value = menu[current_menu_entry_index].minimum;
    } else {
      current_menu_entry_index += encoder_position - last_encoder_position;
      if (current_menu_entry_index < 0) current_menu_entry_index += NUMBER_OF_MENU_ENTRIES;
      current_menu_entry_index %= NUMBER_OF_MENU_ENTRIES;
      /*
      if (current_menu_entry_index < 0) current_menu_entry_index = 0;
      if (current_menu_entry_index > (NUMBER_OF_MENU_ENTRIES - 1)) current_menu_entry_index = (NUMBER_OF_MENU_ENTRIES - 1);
      */
    }
    last_encoder_change_usec = now_usec;
    menu_needs_redraw = true;
    last_encoder_position = encoder_position;
  }
  
  if (menu_needs_redraw && now_usec - last_encoder_change_usec > 2e5L)
  {
    draw_menu();
    menu_needs_redraw = false;
  }

  // DEBUG OUTPUT
  
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
