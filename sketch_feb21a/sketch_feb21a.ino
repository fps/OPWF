#include <TimerOne.h>
#include <EncoderButton.h>

#pragma GCC diagnostic error "-Wall"

EncoderButton encoder(33, 31, 35);

const long timer_period_usec = 10L;

const long steps_per_turn = 200L;
const long microsteps = 16L;

long rounds_per_second = 1L;
long winds = 1000L;
long acceleration = 1L;

void show_help() {
  Serial.println("#########################################################################");
  Serial.println("Commands:");
    Serial.print("w [winds]                        - Set number of winds   ["); Serial.print(winds); Serial.println("]");
    Serial.print("r [rounds per second]            - Set rounds per second ["); Serial.print(rounds_per_second); Serial.println("]");
    Serial.print("a [rounds per second per second] - Set acceleration      ["); Serial.print(acceleration); Serial.println("]");
  Serial.println("s                                - Start winding");
  Serial.println("o                                - Stop winding");
  Serial.println("?                                - show this help");
}

void setup() {
  Serial.begin(115200);

  show_help();

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
