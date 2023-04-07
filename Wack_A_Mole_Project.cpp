//Note: PR is for PhotoResistor
//Fix the first line in the ADC_read16b() function/method
#include "fsl_device_registers.h"

unsigned char charC[10] = {0xBE, 0xA0, 0x3D, 0xB9, 0xA3, 0x9B, 0x9F, 0xB0, 0xBF, 0xBB};
unsigned char charD[10] = {0x7E, 0x60, 0x3D, 0x79, 0x63, 0x5B, 0x5F, 0x70, 0x7F, 0x7B};
unsigned int testVal = 0;
unsigned long Delay = 0x011111;

unsigned int led_states[3] = {0, 0, 0};

enum SM_States{SM_Start, SM_Idle, SM_Game, SM_Display};
int state = SM_Start;
int counter = 0;
int score = 0;
int high_score = 0;

int p_input = 0;
int previous_led = 4;

unsigned short ADC_read16b(int input){
	switch(input){
		case 0: //Arrow input
			ADC0_SC1A = GPIOB_PDIR & 0x04;
			break;
		case 1: //Blue input
			ADC0_SC1A = GPIOB_PDIR & 0x08;
			break;
		case 2: //Green input
			ADC0_SC1A = 0x00001;
			break;
		case 3: //Yellow input
			ADC0_SC1A = 0x00000;
			break;
//		case 4: //Red input
//			ADC0_SC1A = GPIOC_PDIR & 0x800;
//			break;
		default:
			break;
	}
	//ADC0_SC1A = GPIOB_PDIR & 0x04; //SOME INPUT (please come back and change this part to whatever it might need)
	while(ADC0_SC2 & ADC_SC2_ADACT_MASK); //Conversion in progress
	while(!(ADC0_SC1A & ADC_SC1_COCO_MASK)); // Until conversion is complete
	int test = ADC0_RA; //Variable used for debugging
	return ADC0_RA;
}

void generate(){ //if no led is on, turn one randomly one
	int flag = 0;
	int led = 0;

	for(int i = 0; i < 3; i++) {
		if(led_states[i] == 1) {
			flag = 1;
		}
	}

	if (!flag) {

		led = ADC_read16b(0) % 3;

		while(led == previous_led) {
			led = ADC_read16b(0) % 3;
		}

		led_states[led] = 1;

		switch(led){
		case 0: //blue led turn on Port C Pin 9
			GPIOC_PSOR = 0x0200;
			break;
		case 1: //green led turn on Port C Pin 12
			GPIOC_PSOR = 0x1000;
			break;
		case 2: //yellow led turn on Port B Pin 9
			GPIOB_PSOR = 0x0200;
			break;
		default:
			break;
		}
	}
}

void hit() {
//	if((ADC_read16b(0) < 1000) && led_states[0]) {
//		score++;
//	}
	if((ADC_read16b(1) > 35000) && led_states[0]) {
		score++;
		led_states[0] = 0;
		GPIOC_PCOR = 0x0200;
		previous_led = 0;
	}
	else if((ADC_read16b(2) > 20000 ) && led_states[1]) {
		score++;
		led_states[1] = 0;
		GPIOC_PCOR = 0x1000;
		previous_led = 1;
	}
	else if((ADC_read16b(3) > 15000) && led_states[2]) {
		score++;
		led_states[2] = 0;
		GPIOB_PCOR = 0x0200;
		previous_led = 2; //marker to stop repeating lights from generating
	}
//	else if((ADC_read16b(4) > 0) && led_states[4]) {
//		score++;
//	}
}

void display(unsigned int value) { //value will always ranged between 0-99
	GPIOC_PCOR = 0xBF; //Clears output on port C, pins 7 & 5-0;
	GPIOD_PCOR = 0x7F; //Clears output on port D, pins 6-0;

	GPIOC_PSOR = charC[(value / 10) % 10]; //Outputs corresponding value to 7 segment ten's place
	GPIOD_PSOR = charD[(value) % 10]; //Output corresponding value to 7 segment to one's place
}

void software_delay(unsigned long delay){
	while(delay > 0) delay--;
}


int tickFct(int SM_State){
	switch(SM_State){//transitions
		case SM_Start:
			SM_State = SM_Idle;
			break;
		case SM_Idle:
			GPIOC_PSOR = 0x0100;
			if((p_input) && !(ADC_read16b(0) < 1000)){
				SM_State = SM_Idle;
			}
			else if(!(p_input) && (ADC_read16b(0) < 1000)) {
				SM_State = SM_Game;
				generate();
				p_input = 1; //reset button
				break;
			}

			if((ADC_read16b(0) < 1000)){
				p_input = 1;
			}
			else {
				p_input = 0; //reset button
			}
			break;
		case SM_Game:
			GPIOC_PCOR = 0x0100;
			if(counter < 450) { //15 seconds
				counter++;
				if((p_input == 1) && (ADC_read16b(0) < 1000)) {
					SM_State = SM_Game;
				}
				else if((p_input == 0) && (ADC_read16b(0) < 1000)){
					SM_State = SM_Idle;
					counter = 0;
				}
				else{
					SM_State = SM_Game;
				}

				if((ADC_read16b(0) < 1000)){
					p_input = 1;
				}
				else {
					p_input = 0; //reset button
				}

			}
			else{
				SM_State = SM_Display;
				counter = 0;
			}
			break;
		case SM_Display:
			GPIOC_PCOR = 0x0100;
			if(counter < 150) {
				SM_State = SM_Display;
			}
			else {
				counter = 0;
				SM_State = SM_Idle;
				if(high_score < score) {
					high_score = score;
				}
			}
			counter++;
			break;
		default:
			break;
	}

	switch(SM_State){//state actions
		case SM_Start:
			break;
		case SM_Idle:
			previous_led = 4; //reset marker to maker
			led_states[0] = 0;
			led_states[1] = 0;
			led_states[2] = 0;
			GPIOC_PCOR = 0x1200;
			GPIOB_PCOR = 0x0200;
			display(high_score);
			score = 0;
			break;
		case SM_Game:
			generate();
			hit();
			display(score);
			break;
		case SM_Display:
			GPIOC_PCOR = 0x1200;
			GPIOB_PCOR = 0x0200;
			display(score);
			break;
		default:
			break;
		}

	return SM_State;
}


void main(void){
	SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK; //Pins 2,3,10,11 are used for the analog input PR
	SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK; //Pins 11 is used for analog input PR, Pins 0-5 & 7 are used for 7 segment display ten's place
	SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK; //Out pins 0-6, drive the 7 segment display one's place

	PORTB_GPCLR = 0x0E0C0100; //Pins 2, 3, 10, 11 are configured to GPIO PortB
	PORTC_GPCLR = 0x1BBF0100; //Pins 11, 7, 5-0 are configured to GPIO PortC
	PORTD_GPCLR = 0x007F0100; //Pins 6-0 are configured to GPIO PortD

	GPIOB_PDDR = 0x00000200; //Pins 2, 3, 10, 11 are configured to inputs
	GPIOC_PDDR = 0x000013BF; //Pin 11 is configured to input; Pins 7 & 5-0 are configured to outputs
	GPIOD_PDDR = 0x0000007F; //Pins 6-0 are configured to output


	SIM_SCGC6 |= SIM_SCGC6_ADC0_MASK; // Enable ADC0 Clock
	ADC0_CFG1 = 0x0C; //16 bit ADC; Bus Clock
	ADC0_SC1A = 0x1F; //Disable the module, ADCH = 1111;

//	for(int i = 0; i < 100; i++) {
//		software_delay(Delay);
//		display(i); //displays simple counter
//	}


	while(1){
		state = tickFct(state);
		//generate();
		//testVal = ADC_read16b(0);
		//display(testVal);
		for(int i = 0; i < 2; i++) {// 100 milliseconds tick (i=150 = 5 seconds, i = 3 = 0.1 seconds)
			software_delay(Delay);
		}
	}


}