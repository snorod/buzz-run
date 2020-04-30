#include "mbed.h"
#include "uLCD_4DGL.h"
#include "rtos.h"
#include "MMA8452.h"

#define LEFT_X 45
#define CENTER_X 65
#define RIGHT_X 85

#define JUMP_Y 96
#define CENTER_Y 108
#define DUCK_Y 120

#define RADIUS 4

#define SPEED 5
#define VOLUME 0.1

Serial pc(USBTX,USBRX); // included for debugging accelerometer
MMA8452 acc(p28, p27, 100000);
uLCD_4DGL uLCD(p9,p10,p11); // serial tx, serial rx, reset pin;
PwmOut jack(p23);
DigitalIn jump_button(p22);
DigitalIn duck_button(p21);

// mutex for writing to the uLCD
Mutex LCD;

volatile int score;
volatile int high_score;
volatile int steps;
volatile int strikes;
double acc_x;
volatile int player_x;
volatile int player_y;

volatile int obstacle_x;
volatile int obstacle_y;
volatile int dodge_x;

int ob_x_roots[] = {61, 65, 69};
const char* strikes_arr[4] = { "   ", "  X", " XX", "XXX" };
enum ob_type {dodge, duck, jump};
ob_type ob_type_arr[] = {dodge, duck, jump};

volatile ob_type obstacle_type;

volatile bool play_leftright;
volatile bool play_jump;
volatile bool play_duck;
volatile bool play_strikes;
volatile bool play_dead;

void read_sound(void const *argument) {
    while(1) {

        // different sounds in order of priority to audio output
        if (play_dead) {
            jack.period(1.0/440.0);
            jack = VOLUME/2.0;
            Thread::wait(750);
            jack = 0.0;
            Thread::wait(100);
            jack = VOLUME/2.0;
            Thread::wait(750);
            jack = 0.0;
            Thread::wait(100);
            jack = VOLUME/2.0;
            Thread::wait(750);
            jack = 0.0;
            play_dead = false;
            play_strikes = false;
            play_duck = false;
            play_jump = false;
            play_leftright = false;
        } else if (play_strikes) {
            jack.period(1.0/440.0);
            jack = VOLUME/2.0;
            Thread::wait(750);
            jack = 0.0;
            play_strikes = false;
            play_duck = false;
            play_jump = false;
            play_leftright = false;
        } else if (play_duck) {
            jack.period(1.0/130.8);
            jack = VOLUME/2.0;
            Thread::wait(1000);
            jack = 0.0;
            play_duck = false;
            play_jump = false;
            play_leftright = false;
        } else if (play_jump) {
            jack.period(1.0/261.6);
            jack = VOLUME/2.0;
            Thread::wait(150);
            jack.period(1.0/523.3);
            Thread::wait(300);
            jack = 0.0;
            play_jump = false;
            play_leftright = false;
        } else if (play_leftright) {
            jack.period(1.0/261.6);
            jack = VOLUME/2.0;
            Thread::wait(250);
            jack = 0.0;
            play_leftright = false;
        }
    }
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
            play_leftright = true;
            Thread::wait(500);
        } else if (acc_x < -0.5) {
            if (player_x == CENTER_X) {
                player_x = RIGHT_X;
            } else if (player_x == LEFT_X) {
                player_x = CENTER_X;
            }
            play_leftright = true;
            Thread::wait(500);
        }
        Thread::wait(250);
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
    }
}

void read_high_score(void const *argument) {
    while(1) {
        LCD.lock();
        uLCD.locate(0, 0);
        uLCD.printf("HIGH:\n%D", high_score);
        LCD.unlock();
        Thread::wait(1500);
    }
}

void read_strikes(void const *argument) {
    while(1) {
        LCD.lock();
        uLCD.locate(15, 1);
        uLCD.printf(strikes_arr[strikes % 4]);
        LCD.unlock();
        Thread::wait(1000);
    }
}

// check for button press
void read_duck(void const *argument) {
    while(1) {
        if (!duck_button) {
            player_y = DUCK_Y;
            play_duck = true;
            Thread::wait(1000);
            player_y = CENTER_Y;
        }
    }
}

// check for button press
void read_jump(void const *argument) {
    while(1) {
        if (!jump_button && player_y != DUCK_Y) {
            player_y = JUMP_Y;
            play_jump = true;
            Thread::wait(1000);
            if (player_y == JUMP_Y) {
                player_y = CENTER_Y;
            }
        }
    }
}

// check if it's game over
void read_alive(void const *argument) {
    while(1) {
        if (strikes >= 3 || score >= 9999900) {
            if (score > high_score) {
                high_score = score;
            }
            score = 0;
            strikes = 0;
            player_x = CENTER_X;
            player_y = CENTER_Y;
        }
    }
}

void read_position(void const *argument) {
    while(1) {
        LCD.lock();

        // cover up positions on screen where there's no player or obstacle
        if (player_x != LEFT_X) {
            if (obstacle_y != JUMP_Y) {
                uLCD.filled_circle(LEFT_X, JUMP_Y, RADIUS, 0x003057);
            }
            if (obstacle_y != CENTER_Y) {
                uLCD.filled_circle(LEFT_X, CENTER_Y, RADIUS, 0x003057);
            }
            if (obstacle_y != DUCK_Y) {
                uLCD.filled_circle(LEFT_X, DUCK_Y, RADIUS, 0x003057);
            }
        }
        if (player_x != CENTER_X) {
            if (obstacle_y != JUMP_Y) {
                uLCD.filled_circle(CENTER_X, JUMP_Y, RADIUS, 0x003057);
            }
            if (obstacle_y != CENTER_Y) {
                uLCD.filled_circle(CENTER_X, CENTER_Y, RADIUS, 0x003057);
            }
            if (obstacle_y != DUCK_Y) {
                uLCD.filled_circle(CENTER_X, DUCK_Y, RADIUS, 0x003057);
            }
        }
        if (player_x != RIGHT_X) {
            if (obstacle_y != JUMP_Y) {
                uLCD.filled_circle(RIGHT_X, JUMP_Y, RADIUS, 0x003057);
            }
            if (obstacle_y != CENTER_Y) {
                uLCD.filled_circle(RIGHT_X, CENTER_Y, RADIUS, 0x003057);
            }
            if (obstacle_y != DUCK_Y) {
                uLCD.filled_circle(RIGHT_X, DUCK_Y, RADIUS, 0x003057);
            }
        }
        if (player_y != JUMP_Y && obstacle_y != JUMP_Y) {
            if (player_x == RIGHT_X) {
                uLCD.filled_circle(player_x, JUMP_Y, RADIUS, 0x003057);
            } else if (player_x == LEFT_X) {
                uLCD.filled_circle(player_x, JUMP_Y, RADIUS, 0x003057);
            } else if (player_x == CENTER_X) {
                uLCD.filled_circle(player_x, JUMP_Y, RADIUS, 0x003057);
            }
        }
        if (player_y != CENTER_Y && obstacle_y != CENTER_Y) {
            uLCD.filled_circle(player_x, CENTER_Y, RADIUS, 0x003057);
        }
        if (player_y != DUCK_Y && obstacle_y != DUCK_Y) {
            if (player_x == RIGHT_X) {
                uLCD.filled_circle(player_x, DUCK_Y, RADIUS, 0x003057);
            } else if (player_x == LEFT_X) {
                uLCD.filled_circle(player_x, DUCK_Y, RADIUS, 0x003057);
            } else if (player_x == CENTER_X) {
                uLCD.filled_circle(player_x, DUCK_Y, RADIUS, 0x003057);
            }
        }
        uLCD.filled_circle(player_x, player_y, RADIUS, 0xB3A369);
        LCD.unlock();
    }
}

void read_obstacle(void const *argument) {
    while(1) {
        LCD.lock();
        uLCD.filled_rectangle(obstacle_x - 10 - (steps * 2), obstacle_y + 7, obstacle_x + 10 + (steps * 2), obstacle_y - 7, 0x003057);
        steps++;
        obstacle_y = obstacle_y + 12;

        // move obstacle along slope down the track
        if (obstacle_type == dodge) {
            if (dodge_x >= ob_x_roots[2]) {
                dodge_x = dodge_x + 2;
            } else if (dodge_x <= ob_x_roots[0]) {
                dodge_x = dodge_x - 2;
            }
        }

        // reset obstacle if it has exited the screen
        if (obstacle_y >= 130) {
            obstacle_type = ob_type_arr[rand() % 3];
            obstacle_x = ob_x_roots[1];
            if (obstacle_type == dodge) {
                dodge_x = ob_x_roots[rand() % 3];
            }
            obstacle_y = 12;
            steps = 0;
        }

        // draw dodge obstacle
        if (obstacle_type != dodge) {
            uLCD.filled_rectangle(obstacle_x - 10 - (steps * 2), obstacle_y + 7, obstacle_x + 10 + (steps * 2), obstacle_y - 7, 0xEAAA00);
        } else {
            if (dodge_x >= ob_x_roots[2]) {
                // draw rectangle on left side, dodge on the right
                uLCD.filled_rectangle(obstacle_x - 10 - (steps * 2), obstacle_y + 7, dodge_x - 10, obstacle_y - 7, 0xEAAA00);
            } else if (dodge_x <= ob_x_roots[0]) {
                // draw rectangle on right side, dodge on the left
                uLCD.filled_rectangle(dodge_x + 10, obstacle_y + 7, obstacle_x + 10 + (steps * 2), obstacle_y - 7, 0xEAAA00);
            } else {
                // draw two rectangles on either side, dodge in the middle
                uLCD.filled_rectangle(ob_x_roots[0] - 5 - (steps * 2), obstacle_y + 7, ob_x_roots[0] - steps, obstacle_y - 7, 0xEAAA00);
                uLCD.filled_rectangle(ob_x_roots[2] + steps, obstacle_y + 7, ob_x_roots[2] + 5 + (steps * 2), obstacle_y - 7, 0xEAAA00);
            }
        }

        // draw duck/jump obstacle
        if (obstacle_type == duck) {
            uLCD.filled_rectangle(obstacle_x - 8 - (steps * 2), obstacle_y + 7, obstacle_x + 8 + (steps * 2), obstacle_y - 5, 0x003057);
        } else if (obstacle_type == jump) {
            uLCD.filled_rectangle(obstacle_x - 8 - (steps * 2), obstacle_y + 5, obstacle_x + 8 + (steps * 2), obstacle_y - 7, 0x003057);
        }

        // check for collisions, increment strikes if so
        if (obstacle_y == CENTER_Y) {
            if ((obstacle_type == duck && player_y != DUCK_Y) || (obstacle_type == jump && player_y != JUMP_Y) || (obstacle_type == dodge && dodge_x != player_x)) {
                strikes++;
                if (strikes >= 3) {
                    play_dead = true;
                } else {
                    play_strikes = true;
                }
            }
        }
        LCD.unlock();
        Thread::wait(SPEED);
    }
}

int main()
{
    uLCD.background_color(0x003057);
    uLCD.cls();
    uLCD.text_width(4);
    uLCD.text_height(4);
    uLCD.textbackground_color(0x003057);
    uLCD.color(0xB3A369);
    uLCD.printf("\nBuzz Run");

    wait(2);
    uLCD.cls();
    uLCD.baudrate(3000000); // high baud rate for fast refresh

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
    high_score = 0;
    strikes = 0;
    steps = 0;

    // make sure sounds are off
    play_leftright = false;
    play_jump = false;
    play_duck = false;
    play_strikes = false;
    play_dead = false;

    // initialize player position
    player_x = CENTER_X;
    player_y = CENTER_Y;

    // initialize obstacle coordinates
    obstacle_x = ob_x_roots[1];
    dodge_x = ob_x_roots[rand() % 3];
    obstacle_y = 12;
    obstacle_type = ob_type_arr[rand() % 3];

    // pull up push_buttons
    jump_button.mode(PullUp);
    duck_button.mode(PullUp);

    // start threads
    Thread acc_thread(read_acc);
    Thread score_thread(read_score);
    Thread duck_thread(read_duck);
    Thread jump_thread(read_jump);
    Thread alive_thread(read_alive);
    Thread position_thread(read_position);
    Thread obstacle_thread(read_obstacle);
    Thread strikes_thread(read_strikes);
    Thread sound_thread(read_sound);
    Thread high_score_thread(read_high_score);

    while(1) {
        // loop forever
    }

}
