/*Pistetavoite 1-2. Tehtävistä tehty osat yksi ja kaksi, 
jotka oikeuttaisivat kahteen pisteeseen, mutta palautus 
tehty myöhässä hankalan elämäntilanteen vuoksi.*/

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <inttypes.h>

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

// Led threads initialization
#define STACKSIZE 500
#define PRIORITY 5
int init_led();
void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);

// Configure buttons
#define BUTTON_0 DT_ALIAS(sw0)
// #define BUTTON_1 DT_ALIAS(sw1)
static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static struct gpio_callback button_0_data;

enum state {
	RED,
	YELLOW,
	GREEN,
	PAUSE
};

volatile enum state state = RED;
volatile enum state prev_state = RED;

enum direction {
	UP,
	DOWN
};

volatile enum direction direction = DOWN;

// Main program
int main(void)
{
	init_led();

	int ret = init_button();
	if (ret < 0) {
		return 0;
	}

	while (1) {
		k_msleep(10); // sleep 10ms
	}

	return 0;
}

// Button interrupt handler
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (state != PAUSE) {
		prev_state = state;
		state = PAUSE;
		printk("PAUSED\n");
	} 
	else {
		state = prev_state;
		printk("RESUMED\n");
	}

	printk("Button pressed\n");
}

// Button initialization
int init_button() {

	int ret;
	if (!gpio_is_ready_dt(&button_0)) {
		printk("Error: button 0 is not ready\n");
		return -1;
	}

	ret = gpio_pin_configure_dt(&button_0, GPIO_INPUT);
	if (ret != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&button_0_data, button_0_handler, BIT(button_0.pin));
	gpio_add_callback(button_0.port, &button_0_data);
	printk("Set up button 0 ok\n");
	
	return 0;
}

// Initialize leds
int init_led() {

	int ret;

	// Red led pin initialization
	ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Red configure failed\n");		
		return ret;
	}

	// Green led pin initialization
	ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Green configure failed\n");		
		return ret;
	}

	// Blue led pin initialization
	ret = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Blue configure failed\n");		
		return ret;
	}

	// set led off
	gpio_pin_set_dt(&red,0);
	gpio_pin_set_dt(&green,0);
	gpio_pin_set_dt(&blue,0);

	printk("Leds initialized ok\n");
	
	return 0;
}

// Task to handle red led
void red_led_task(void *, void *, void*) {
	
	printk("Red led thread started\n");
	while (true) {
		if (state == PAUSE) {
        	k_msleep(100);
        	continue;
    	}
		if (state == RED) {
			gpio_pin_set_dt(&red,1);
			printk("Red on\n");
			k_sleep(K_SECONDS(1));
			if (state == PAUSE){
				while (state == PAUSE){
					k_msleep(100);
				}
			}
			gpio_pin_set_dt(&red,0);
			printk("Red off\n");
			state = YELLOW;
			direction = DOWN;
		}
		k_msleep(100);
	}
}

// Task to handle yellow led
void yellow_led_task(void *, void *, void*) {
	
	printk("Yellow led thread started\n");
	while (true) {
		if (state == PAUSE) {
        	k_msleep(100);
        	continue;
    	}
		if (state == YELLOW) {
			gpio_pin_set_dt(&red,1);
			gpio_pin_set_dt(&green,1);
			printk("yellow on\n");
			k_sleep(K_SECONDS(1));
			if (state == PAUSE){
				while (state == PAUSE){
					k_msleep(100);
				}
			}
			gpio_pin_set_dt(&red,0);
			gpio_pin_set_dt(&green,0);
			printk("yellow off\n");
			state = GREEN;
			if (direction == UP) {
				state = RED;
			}
		}	
		k_msleep(100);
	}
}

// Task to handle green led
void green_led_task(void *, void *, void*) {
	
	printk("Green led thread started\n");
	while (true) {
		if (state == PAUSE) {
        	k_msleep(100);
        	continue;
    	}
		if (state == GREEN) {
			gpio_pin_set_dt(&green,1);
			printk("Green on\n");
			k_sleep(K_SECONDS(1));
			if (state == PAUSE){
				while (state == PAUSE){
					k_msleep(100);
				}
			}
			gpio_pin_set_dt(&green,0);
			printk("Green off\n");
			direction = UP;
			if (direction == UP) {
				state = YELLOW;
			}
		}
		k_msleep(100);
	}
}

