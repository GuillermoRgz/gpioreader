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
#define GPIO_CHIP 	"gpiochip0"
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
    while (1) {
        // Wait for an event with a timeout (e.g., 5 seconds)
        struct timespec timeout = {5, 0}; // 5 seconds
        int ret = gpiod_line_event_wait(input_line, &timeout);

        if (ret < 0) {
            perror("Error waiting for GPIO event");
            break; // Exit the loop on error
        } else if (ret == 0) {
            printf("Timeout waiting for GPIO event\n");
            continue; // Timeout occurred; no event detected
        }

        // Read the event
        struct gpiod_line_event event;
        ret = gpiod_line_event_read(input_line, &event);
        if (ret < 0) {
            perror("Error reading GPIO event");
            break; // Exit the loop on error
        }

        // Handle the event
        if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
            printf("Rising edge detected\n");
            gpiod_line_set_value(output_line, 1); // Set output to HIGH
        } else if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
            printf("Falling edge detected\n");
            gpiod_line_set_value(output_line, 0); // Set output to LOW
        } else {
            printf("Unknown event type\n");
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
    if (output_line) gpiod_line_release(output_line);
    if (input_line) gpiod_line_release(input_line);
    if (chip) gpiod_chip_close(chip);
    exit(0);
}
