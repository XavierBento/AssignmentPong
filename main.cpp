#include <mbed.h>
#include <MMA7455.h>
#include <LM75B.h>
#include <display.h>
#include <stdlib.h>

// Initialise Joystick   
typedef enum {JLEFT = 0, JRIGHT, JUP, JDOWN, JCENTER} btnId_t;
static DigitalIn jsBtns[] = {P5_4, P5_0, P5_2, P5_1, P5_3}; // LFT, RGHT, UP, DWN, CTR
bool jsPrsdAndRlsd(btnId_t b);
bool jsPrsd(btnId_t b);

//Input object for the potentiometer
AnalogIn pot(p15);

//Global Variable
bool mt = false; 
bool scoreDoubble = false; 
static int collisions = 0; 

//Initialise the screen for the game
Display *screen = Display::theDisplay(); 

//Struct for the Bat
typedef struct Bat { 
	static const int y = 258;  
	static const int width = 40; 
	static const int depth = 4; 
	int x; 
	int vx; 
	int colour; // Bat color (0 - white or 1 - black)
} bat_t;

//Ball Struct
typedef struct Ball { 
	int x, y; 
	int vx, vy; 
	int colour; 
	int scoreRate; 
} ball_t;


//Obstacle Struct
typedef struct Obstacle { 
	int x; 
	int y; 
	int width; 
	static const int depth = 4; 
} obstacle_t;


//Instantiation of functions for the game
void initGame(); 
void initBatBall(ball_t &tBall, bat_t &tBat); 
void initObstacle(obstacle_t &obstcl); 
void updStatusBar(int bRemaining, int Scr); 

void waitForStart(); 
void renderBall(ball_t ball); 
void updateBall(ball_t &ball); 
void renderBat(bat_t &b); 
void updBatSpeed(bat_t &bat);

int collBottom(ball_t ball);
int collTop(ball_t &ball, int scrPoint);

void collLeft(ball_t &ball);
void collRight(ball_t &ball);
void collBat(ball_t &ball, bat_t b);

void collObs(obstacle_t obstcl, ball_t &ball);

void checkMagicTime(int &r, ball_t &ball);

void timerHandler(); 
int tickCt = 0;

int main() {
	//start the ticker counter
	Ticker tickTock;
	tickTock.attach(&timerHandler, 1);
	
	int rmd, ballInPlay, ballsRem, score;
	ball_t ball;
	bat_t bat;
	obstacle_t obstacle;
	
	//loop that cause game to reset when out of balls
	while (true){
		ballsRem = 5; 
		score = 0;
		initGame(); 
		
		
		while ( ballsRem > 0 ) {
			
			
			waitForStart();
			ballInPlay= 1;
			initBatBall(ball, bat);
			initObstacle(obstacle);
			renderBall(ball);
			updateBall(ball);
			renderBat(bat);		
			
			//choses a random time between 5 and 10 seconds for magic time to begin after
			rmd = 5 + rand()%5;
			
			//loop that updates the screen while the ball is in play
			while ( ballInPlay ) {
				
				updBatSpeed(bat);
				renderBall(ball);
				updateBall( ball );
				renderBat(bat);		
				ballInPlay = collBottom(ball);
				
				//check for collisions if there is a ball in play
				if (ballInPlay != 0) {
					
					collObs(obstacle, ball);
					collBat(ball, bat);
					collLeft(ball);
					collRight(ball);
					score = collTop(ball, score);
					checkMagicTime(rmd, ball);
					
				}
				
				if (ballInPlay == 0) {
					ballsRem -= 1; 
				}
				//redraw status bar with updated values
				updStatusBar(ballsRem, score);
			}
		}
		//Game over screen
		screen->fillScreen(WHITE);
		screen->setCursor(130,80);
		screen->printf("Game Over! Your final score is: %d", score);
		screen->setCursor(130,90);
		screen->printf("Press Joystick Center to begin.");
		
		//wait for joystick center before starting a new game
		waitForStart();
	}
}

void waitForStart() {
	while (jsPrsdAndRlsd(JCENTER) == false) {
		//constant busy-wait in loop until Joystick CENTRE is pressed
	}
}
	
//Draws the game court
void initGame() { 
	screen->fillScreen(WHITE);
  screen->setTextColor(BLACK, WHITE); 
	screen->drawRect(0,0,480,16,BLACK);
	updStatusBar(0,0);
}

//Create the status bar, Constant updates for the score and remaining balls
void updStatusBar(int bRemaining,int scr) { 
	screen->setCursor(4,4);
	screen->printf("Balls Remaining: %d", bRemaining);
	screen->setCursor(380,4);
	screen->printf("Score: %d", scr);
}
	
//Initialisation of the bat & Ball with random positions
void initBatBall(ball_t &tBall, bat_t &tBat) { 
	int xv = 0, yv = 0, paddleXPos; // Define velocity x,y and paddle x position
	//reset the magic time ticker count for this ball
	tickCt = 0;
	screen->fillRect(0,55,480,220, WHITE);
	//Initalisation of the ball with a random xv, yv, that are different from zero
	do {
		yv = -3 + rand() % 6; 
	} while (yv == 0);
	do {
		xv = -4 + rand() % (8);
	} while ((xv == 0) && (xv < yv));
	
	//Ball Randomisation code
	tBall.x = 10 + rand() % 450; 
	tBall.y = 60;
	tBall.vx = xv;
	tBall.vy = yv; 
	tBall.colour = 1; 
	tBall.scoreRate = 0;
	
	//reset the global collisions count for this ball
	collisions = 0;
	paddleXPos = rand() % 440;
	
	//Initialisation of the bat
	tBat.x = paddleXPos;	
	tBat.vx = 3;
	tBat.colour = 1;
}

//Initalise the obstacle in a random position and random length
void initObstacle(obstacle_t &obstcl){ 
	//Obstacle randomisation
	obstcl.x = 10 + rand() % 450; 
	obstcl.y = 60 + rand() % 100;
	obstcl.width = 40 + rand() % 160;
	//Renders the obstacle on the court
	screen->fillRect(obstcl.x, obstcl.y, obstcl.width, obstcl.depth, GREEN);

}
//colour 0 means white 1 means blue and 2 means red, renders the ball on screen in correct colour
void renderBall(ball_t ball){
	int colour = ball.colour;
	if (colour == 0) {
		screen->fillCircle(ball.x, ball.y, 5, WHITE);
	}
	if (colour == 1) {
		screen->fillCircle(ball.x, ball.y, 5, BLUE);
		wait(0.01);
	}
	else if (colour == 2) {
		screen->fillCircle(ball.x, ball.y, 5, RED);
		wait(0.01);
	}
}

//update ball position for next render and collision checks 
void updateBall(ball_t &ball) { 
	int colState = ball.colour; 
	ball.colour = 0;
	renderBall(ball);
	ball.colour = colState;
	ball.x += ball.vx;
	ball.y += ball.vy;
}

//render the bat and updates it as joystick left or right is pressed 
void renderBat(bat_t &b){ 
		screen->fillRect(b.x, b.y, b.width, b.depth, BLACK);
	if (jsPrsd(JLEFT)) {
		screen->fillRect(b.x, b.y, b.width, b.depth, WHITE);
		if (b.x - b.vx >= 0){
			b.x -= b.vx;
		}
		screen->fillRect(b.x, b.y, b.width, b.depth, BLACK);
	}
	if (jsPrsd(JRIGHT)) {
		screen->fillRect(b.x, b.y, b.width, b.depth, WHITE);
		if (b.x + b.vx < 440){
			b.x += b.vx;
		}
		screen->fillRect(b.x, b.y, b.width, b.depth, BLACK);
	}
}

//updates the speed of the bat based off the value of the potentiometer
void updBatSpeed(bat_t &bat) {
	float speedB = pot.read() * 10;
	int vx = 1 + int(speedB);
	bat.vx = vx;
	//print in the status bar the speed of the ball
	screen->setCursor(170, 4);
	screen->printf("Bat Speed: %2d", vx);
}

void collBat(ball_t &ball, bat_t bat) { //collsion for BAT detection the BALL
	if ((bat.x <= (ball.x + 5) && //checks right hand side of ball is within bat
			(ball.x - 5) <= (bat.x + bat.width)) && //
			ball.y + 6 == bat.y) //
	{
		ball.vy = - ball.vy;
	}
}

//checks for a collision with the bottom of the screen
int collBottom(ball_t ball){
	int result = 1;
	if ((ball.y) > 262) {
		result = 0; 
	}
	return result;
}
//checks for a collision with the status bar
int collTop(ball_t &ball, int scrPoint){
	if (ball.y - 5 < 18) {
		collisions++;
		ball.vy = - ball.vy;
		if (collisions % 5 == 0){
			ball.scoreRate = collisions/5;
		}
		if (scoreDoubble) {
			scrPoint += (1 + ball.scoreRate)*2;
		}
		else {
			scrPoint += 1 + ball.scoreRate;
		}
	}
	return scrPoint;
}
//checks for a collision with the left of the screen
void collLeft(ball_t &ball){
	if ((ball.x - 5) <= 0) {
		ball.vx = - ball.vx;
	}
}
//checks for a collision with the right of the screen
void collRight(ball_t &ball){
	if ((ball.x + 5) >= 480) {
		ball.vx = - ball.vx;
	}
}

//checks for a collision with the obstacle
void collObs(obstacle_t obstcl, ball_t &ball){
	//cehcks for collision with top of obstacle 
	if ((ball.y - 6 < obstcl.y && obstcl.y < ball.y + 6) && (ball.x + 6 == obstcl.x)){
		ball.vx = -ball.vx;
	}
	//cehcks for collision with sides of obstacle 
	if ((ball.y - 6 < obstcl.y && obstcl.y < ball.y + 6) && ((ball.x - 6 < (obstcl.x + obstcl.width)) && (ball.x + 6 > obstcl.x) )){
		if (ball.x < obstcl.x + (0.5 * obstcl.width) ) {
			ball.x -= 10;
		}
		if (ball.x > obstcl.x + (0.5 * obstcl.width) ) {
			ball.x += 10;
		}
		
		ball.vx = -ball.vx;
	}
	//cehcks for collision with bottom of obstacle 
	else if ((obstcl.x <= (ball.x + 6) && 
			(ball.x - 6) <= (obstcl.x + obstcl.width)) && 
			(ball.y + 6 == obstcl.y || ball.y - 6 == (obstcl.y + obstcl.depth) )) 
	{
		ball.vy = - ball.vy;
	}
	
}

//checks the count of the timer and uses that to set up magic time in the game
void checkMagicTime(int &r, ball_t &ball) {
	if (tickCt == r && !mt) { //start magic time after a random delay
		tickCt = 0; //reset the count of the timer interupt
		mt = true;
		//set colour of ball to red
		ball.colour = 2;
		scoreDoubble = true;
		r = 2 + rand()%8; //select new random value for duration of magic time
	}
	if (tickCt == r && mt) { //magic time period has ended
		tickCt = 0; //restart the counter 
		r = 5 + rand()%5; //select a new random value for magic time to start after 
		mt = false;
		//set colour of ball to blue
		ball.colour = 1;
		scoreDoubble = false;
	}
}

//Definition of timer interrupt handler
void timerHandler() {
  tickCt++;
}

/* Definition of Joystick press capture function
 * b is one of JLEFT, JRIGHT, JUP, JDOWN - from enum, 0, 1, 2, 3
 * Returns true if this Joystick pressed then released, false otherwise.
 *
 * If the value of the button's pin is 0 then the button is being pressed,
 * just remember this in savedState.
 * If the value of the button's pin is 1 then the button is released, so
 * if the savedState of the button is 0, then the result is true, otherwise
 * the result is false. */

bool jsPrsdAndRlsd(btnId_t b) {
	bool result = false;
	uint32_t state;
	static uint32_t savedState[4] = {1,1,1,1};
        //initially all 1s: nothing pressed
	state = jsBtns[b].read();
  if ((savedState[b] == 0) && (state == 1)) {
		result = true;
	}
	savedState[b] = state;
	return result;
}

//simply checks if a button has been pressed
bool jsPrsd(btnId_t b) {
	bool result = false;
	uint32_t state;
	state = jsBtns[b].read();
	if (state == 0) {
		result = true;
	}
	return result;
}
