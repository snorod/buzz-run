#include "mbed.h"
#include "uLCD_4DGL.h"
#include "rtos.h"
#include "MMA8452.h"

MMA8452 acc(p28, p27, 100000);
uLCD_4DGL uLCD(p9,p10,p11); // serial tx, serial rx, reset pin;

PwmOut jack(p23);
DigitalIn jump_button(p22);
DigitalIn duck_button(p21);

Mutex LCD;
volatile int score;
volatile int strikes;
volatile bool alive;
double acc_x;
volatile int player_x;
volatile int player_y;

volatile int obstacle_x;
volatile int obstacle_y;

int ob_x_roots[] = {55, 65, 75};


#define VOLUME 0.01
#define BPM 100.0

#define LEFT_X 45
#define CENTER_X 65
#define RIGHT_X 85

#define JUMP_Y 96
#define CENTER_Y 108
#define DUCK_Y 120

void playNote(float frequency, float duration, float volume) {
    jack.period(1.0/frequency);
    jack = volume/2.0;
    wait(duration);
    jack = 0.0;
}


void read_acc(void const *argument){
    while(1) {
        acc.readXGravity(&acc_x);
        if (acc_x > 0.5) {
            if (player_x == CENTER_X) {
                player_x = LEFT_X;
            } else if (player_x == RIGHT_X) {
                player_x = CENTER_X;
            }
            Thread::wait(500);
        } else if (acc_x < -0.5) {
            if (player_x == CENTER_X) {
                player_x = RIGHT_X;
            } else if (player_x == LEFT_X) {
                player_x = CENTER_X;
            }
            Thread::wait(500);
        }
        Thread::wait(250);
        //uLCD.printf("%f", acc_x);
        
        //uLCD.cls();
        /*PPM = CO2_sensor * 200.0;
        LCD.lock();
        uLCD.color(WHITE);
        uLCD.locate(1,2);
        uLCD.printf("%3.0f",PPM);
        LCD.unlock();
        old_PPM = PPM;
        Thread::wait(500);
        */
    }
}

void read_score(void const *argument) {
    while(1) {
        score = score + 10;
        LCD.lock();
        uLCD.locate(11, 0);
        uLCD.printf("%7D", score); // maximum 7-digit score, cap at 9,999,900
        LCD.unlock();
        Thread::wait(1000);
        //uLCD.cls();
    }
}

void read_duck(void const *argument) {
    while(1) {
        if (!duck_button) {
            player_y = DUCK_Y;
            Thread::wait(1000);
            player_y = CENTER_Y;
        }
    }
}

void read_jump(void const *argument) {
    while(1) {
        if (!jump_button && player_y != DUCK_Y) {
            player_y = JUMP_Y;
            Thread::wait(1000);
            if (player_y == JUMP_Y) {
                player_y = CENTER_Y;
            }
        }
    }
}

void read_alive(void const *argument) {
    while(1) {
        if (strikes >= 3 || score >= 9999900) {
            // display score and ask for retry
            score = 0;
            strikes = 0;
            player_x = CENTER_X;
            player_y = CENTER_Y;
            alive = true;
        }
    }
}

void read_position(void const *argument) {
    while(1) {
        LCD.lock();
        if (player_x != LEFT_X) {
            uLCD.filled_circle(LEFT_X, JUMP_Y, 5, 0x003057);
            uLCD.filled_circle(LEFT_X, CENTER_Y, 5, 0x003057);
            uLCD.filled_circle(LEFT_X, DUCK_Y, 5, 0x003057);
        }
        if (player_x != CENTER_X) {
            uLCD.filled_circle(CENTER_X, JUMP_Y, 5, 0x003057);
            uLCD.filled_circle(CENTER_X, CENTER_Y, 5, 0x003057);
            uLCD.filled_circle(CENTER_X, DUCK_Y, 5, 0x003057);
        }
        if (player_x != RIGHT_X) {
            uLCD.filled_circle(RIGHT_X, JUMP_Y, 5, 0x003057);
            uLCD.filled_circle(RIGHT_X, CENTER_Y, 5, 0x003057);
            uLCD.filled_circle(RIGHT_X, DUCK_Y, 5, 0x003057);
        }
        if (player_y != JUMP_Y) {
            uLCD.filled_circle(player_x, JUMP_Y, 5, 0x003057);
        }
        if (player_y != CENTER_Y) {
            uLCD.filled_circle(player_x, CENTER_Y, 5, 0x003057);
        }   
        if (player_y != DUCK_Y) {
            uLCD.filled_circle(player_x, DUCK_Y, 5, 0x003057);
        }
        uLCD.filled_circle(player_x, player_y, 5, 0xB3A369);
        LCD.unlock();
        //Thread::wait(250);
    }
}

void read_obstacle(void const *argument) {
    while(1) {
        LCD.lock();
        uLCD.filled_rectangle(obstacle_x - 5, obstacle_y + 5, obstacle_x + 5, obstacle_y - 5, 0x003057);
        obstacle_y = obstacle_y + 12;
        if (obstacle_x >= ob_x_roots[2]) {
            obstacle_x = obstacle_x + 2;
        } else if (obstacle_x <= ob_x_roots[0]) {
            obstacle_x = obstacle_x - 2;
        }
        if (obstacle_y >= 130) {
         obstacle_x = ob_x_roots[rand() % 3];
         obstacle_y = 15;   
        }
        //obstacle_y = obstacle_y % 130;
        uLCD.filled_rectangle(obstacle_x - 5, obstacle_y + 5, obstacle_x + 5, obstacle_y - 5, 0xEAAA00);
        LCD.unlock();
        //obstacle_x = obstacle_x + 10;
        Thread::wait(1000);
    }   
}

int main()
{
    uLCD.background_color(0x003057);
    uLCD.text_width(4); //4X size text
    uLCD.text_height(4);
    uLCD.background_color(0x003057);
    uLCD.textbackground_color(0x003057);
    uLCD.color(0xB3A369);
    uLCD.background_color(0x003057);
    uLCD.printf("Buzz Run");
    wait(1);
    uLCD.cls();
    uLCD.baudrate(3000000); //jack up baud rate to max for fast display
    //if demo hangs here - try lower baud rates
    
    /*
    uLCD.line(23, 130, 40, 28, 0xFFFFFF);
    uLCD.line(24, 130, 43, 16, 0xB3A369);
    uLCD.line(25, 130, 45, 10, 0xB3A369);
    uLCD.line(26, 130, 45, 16, 0xB3A369);
    uLCD.line(27, 130, 44, 28, 0xFFFFFF);
    */
    
    // Left-side guardrails
    uLCD.line(23, 130, 43, 10, 0xFFFFFF);
    uLCD.line(24, 130, 44, 10, 0xB3A369);
    uLCD.line(25, 130, 45, 10, 0xB3A369);
    uLCD.line(26, 130, 46, 10, 0xB3A369);
    uLCD.line(27, 130, 47, 10, 0xFFFFFF);
    
    // Right-side guardrails
    uLCD.line(103, 130, 83, 10, 0xFFFFFF);
    uLCD.line(104, 130, 84, 10, 0xB3A369);
    uLCD.line(105, 130, 85, 10, 0xB3A369);
    uLCD.line(106, 130, 86, 10, 0xB3A369);
    uLCD.line(107, 130, 87, 10, 0xFFFFFF);
    
    score = 0;
    strikes = 0;
    alive = true;
    player_x = CENTER_X;
    player_y = CENTER_Y;
    
    obstacle_x = ob_x_roots[rand() % 3];
    obstacle_y = 15;
    
    //acc.readXGravity(&x_init);
    jump_button.mode(PullUp);
    duck_button.mode(PullUp);
    
    Thread acc_thread(read_acc);
    Thread score_thread(read_score);
    Thread duck_thread(read_duck);
    Thread jump_thread(read_jump);
    Thread alive_thread(read_alive);
    Thread position_thread(read_position);
    Thread obstacle_thread(read_obstacle);
    
    /*float beat_duration;

    // Calculate duration of a quarter note from bpm
    beat_duration = 60.0 / BPM;*/
    while(1) {/*
        // First measure
        playNote(391.995, (beat_duration - 0.1), VOLUME);
        wait(0.1);
        playNote(391.995, (beat_duration - 0.1), VOLUME);
        wait(0.1);
        playNote(391.995, (beat_duration - 0.1), VOLUME);
        wait(0.1);
        playNote(311.127, (0.75 * beat_duration), VOLUME);
        playNote(466.164, (0.25 * beat_duration), VOLUME);*/
    }

}

