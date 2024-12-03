/*
 * main.c
 *
 *  Created on: Nov 17, 2024
 *      Author: Guillermo Rodriguez Rodriguez
 *     Version: 0.1
 *
 * Description: Reads continuously from GPIO 17 and sets GPIO 18 to logic high as response.
 */


// ###  INCLUDE  ######################
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <gpiod.h>

// ###  END INCLUDE  ##################

// ###  DEFINE  #######################
#define GPIO_CHIP 	"/dev/gpiochip0"
#define INPUT_PIN	17
#define OUTPUT_PIN	18

//  ###  END DEFINE  ##################

//  ###  GLOBAL VARIABLES  ############
struct gpiod_chip 		*chip;
struct gpiod_line 		*input_line;
struct gpiod_line 		*output_line;

//  ### END GLOBAL VARIABLES  #########

//  ###  FUNCTION PROTOTYPES  #########
void handle_sigint(int sig);

//  ### END FUNCTIONS  ################

int main(int argc, char *argv[]) {

    struct gpiod_line_event event;
    int ret;

    signal(SIGINT, handle_sigint);

    chip = gpiod_chip_open_by_name(GPIO_CHIP);
    if(!chip){
    	perror("Failed to open GPIO chip.");
    	return 1;
    }

    // Request the output line
    output_line = gpiod_chip_get_line(chip, OUTPUT_PIN);
    if (!output_line) {
        perror("Failed to get output GPIO line");
        gpiod_line_release(input_line);
        gpiod_chip_close(chip);
        return 1;
    }

    ret = gpiod_line_request_output(output_line, "output_driver", 0);
    if (ret < 0) {
        perror("Failed to set output line as output");
        gpiod_line_release(input_line);
        gpiod_chip_close(chip);
        return 1;
    }

    printf("Monitoring GPIO %d for changes...\n",INPUT_PIN);


    // MAIN LOOP ----------------------------------------------
    while(1){

    	ret = gpiod_line_event_wait(input_line, NULL);

    	if (ret < 0){
    		perror("Error waiting for event.");
    		return 1;
    	}

    	// Timeout return
    	if (ret == 0){
    		continue;
    	}

    	ret = gpiod_line_event_read(input_line, &event);

    	if (ret < 0){
    		perror("Error reading event.");
    		return 1;
       	}

    	// Rising event
    	if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE){
    		printf("Rising edge detected on GPIO %d\n", INPUT_PIN);
    		gpiod_line_set_value(output_line, 1);
    	}else if(event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE){
    		printf("Falling edge detected on GPIO %d\n", INPUT_PIN);
    		gpiod_line_set_value(output_line, 0);
    	}
    }

    // END MAIN LOOP ------------------------------------------

    gpiod_line_release(input_line);
    gpiod_line_release(output_line);
    gpiod_chip_close(chip);


	return 0;
}


void handle_sigint(int sig) {
    printf("\nCleaning up and exiting...\n");
    printf("Stopping NOW!!\n");
    if (input_line) gpiod_line_release(input_line);
    if (output_line) gpiod_line_release(output_line);
    if (chip) gpiod_chip_close(chip);
    exit(0);
}
