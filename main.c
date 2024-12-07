#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/gpio.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>

#define GPIO_DEVICE "/dev/gpiochip0" // Adjust if your GPIO chip is different
#define GPIO_INPUT_LINE 26          // GPIO pin to monitor for rising edge
#define GPIO_OUTPUT_LINE 6          // GPIO pin to toggle high for 1ms

// File descriptors for cleanup
int chip_fd = -1;
int input_fd = -1;
int output_fd = -1;

// Signal handler for SIGINT
void handle_sigint(int sig) {
    printf("\nSIGINT received. Cleaning up and exiting...\n");

    // Close file descriptors if open
    if (input_fd >= 0) close(input_fd);
    if (output_fd >= 0) close(output_fd);
    if (chip_fd >= 0) close(chip_fd);

    exit(EXIT_SUCCESS);
}

void set_gpio_high_then_low(int fd, int line_fd) {
    struct gpiohandle_data data;

    // Set the GPIO line to HIGH
    data.values[0] = 1;
    if (ioctl(line_fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data) < 0) {
        perror("Failed to set GPIO line HIGH");
        return;
    }

    // Wait 1 millisecond
    usleep(1000);

    // Set the GPIO line to LOW
    data.values[0] = 0;
    if (ioctl(line_fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data) < 0) {
        perror("Failed to set GPIO line LOW");
    }
}

int main() {
    struct gpioevent_request input_req;
    struct gpiohandle_request output_req;
    struct gpioevent_data event;

    // Register SIGINT handler
    signal(SIGINT, handle_sigint);

    // Open the GPIO chip
    chip_fd = open(GPIO_DEVICE, O_RDONLY);
    if (chip_fd < 0) {
        perror("Failed to open GPIO chip device");
        return EXIT_FAILURE;
    }

    // Request the input GPIO line for event monitoring
    memset(&input_req, 0, sizeof(input_req));
    input_req.lineoffset = GPIO_INPUT_LINE;                // GPIO pin number
    input_req.handleflags = GPIOHANDLE_REQUEST_INPUT;      // Configure as input
    input_req.eventflags = GPIOEVENT_REQUEST_RISING_EDGE;  // Rising edge detection
    strncpy(input_req.consumer_label, "gpio-monitor", sizeof(input_req.consumer_label) - 1);

    if (ioctl(chip_fd, GPIO_GET_LINEEVENT_IOCTL, &input_req) < 0) {
        perror("Failed to request input GPIO line event");
        close(chip_fd);
        return EXIT_FAILURE;
    }
    input_fd = input_req.fd; // Save input line file descriptor for cleanup

    // Request the output GPIO line for control
    memset(&output_req, 0, sizeof(output_req));
    output_req.lineoffsets[0] = GPIO_OUTPUT_LINE;         // GPIO pin number
    output_req.flags = GPIOHANDLE_REQUEST_OUTPUT;         // Configure as output
    output_req.default_values[0] = 0;                     // Initialize to LOW
    strncpy(output_req.consumer_label, "gpio-monitor", sizeof(output_req.consumer_label) - 1);
    output_req.lines = 1;

    if (ioctl(chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &output_req) < 0) {
        perror("Failed to request output GPIO line handle");
        close(input_fd);
        close(chip_fd);
        return EXIT_FAILURE;
    }
    output_fd = output_req.fd; // Save output line file descriptor for cleanup

    printf("Monitoring GPIO pin %d for rising edges. Press Ctrl+C to exit.\n", GPIO_INPUT_LINE);

    // Monitor events
    while (1) {
        int ret = read(input_fd, &event, sizeof(event));
        if (ret < 0) {
            perror("Failed to read GPIO event");
            break;
        }

        // Convert the timestamp to a human-readable format
        struct timespec ts;
        ts.tv_sec = event.timestamp / 1000000000;        // Convert nanoseconds to seconds
        ts.tv_nsec = event.timestamp % 1000000000;       // Remainder is nanoseconds
        char time_buffer[64];
        struct tm *tm_info = localtime(&ts.tv_sec);
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);

        printf("Rising edge detected on GPIO pin %d at %s.%09ld\n",
               GPIO_INPUT_LINE, time_buffer, ts.tv_nsec);

        // Set GPIO output line HIGH for 1ms, then LOW
        set_gpio_high_then_low(output_fd, output_req.fd);
    }

    // Cleanup
    close(input_fd);
    close(output_fd);
    close(chip_fd);
    return EXIT_SUCCESS;
}
