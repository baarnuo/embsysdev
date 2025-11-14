/*Pistetavoite */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <inttypes.h>
#include <zephyr/timing/timing.h>

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

// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// Create dispatcher FIFO buffer
K_FIFO_DEFINE(dispatcher_fifo);
K_FIFO_DEFINE(debug_fifo);

// FIFO dispatcher data type
struct data_t {
	void *fifo_reserved;
	char msg[20];
};

static void dispatcher_task(void *, void *, void *);
static void uart_task(void *, void *, void *);
static void debug_task(void *, void *, void *);

#define UART_PRIORITY 10
#define DEBUG_PRIORITY 4

K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,UART_PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,UART_PRIORITY,0,0);
K_THREAD_DEFINE(debug_thread,STACKSIZE,debug_task,NULL,NULL,NULL,DEBUG_PRIORITY,0,0);

K_SEM_DEFINE(red_sem, 0, 1);
K_SEM_DEFINE(yellow_sem, 0, 1);
K_SEM_DEFINE(green_sem, 0, 1);
K_SEM_DEFINE(release_sem, 0, 1);

// Main program
int main(void)
{
	init_uart();
	init_led();
	timing_init();

	return 0;
}

int init_uart(void) {
	// UART initialization
	if (!device_is_ready(uart_dev)) {
		printk("UART initialization failed\n");
		return 1;
	} 
	printk("UART initialized\n");
	return 0;
}

static void uart_task(void *, void *, void *)
{
	// Received character from UART
	char rc=0;
	// Message from UART
	char uart_msg[20];
	memset(uart_msg,0,20);
	int uart_msg_cnt = 0;

	while (true) {
		// Ask UART if data available
		if (uart_poll_in(uart_dev,&rc) == 0) {
			// printk("Received: %c\n",rc);
			// If character is not newline, add to UART message buffer
			if (rc != '\r') {
				uart_msg[uart_msg_cnt] = rc;
				uart_msg_cnt++;
			// Character is newline, copy dispatcher data and put to FIFO buffer
			} else {
				printk("UART msg: %s\n", uart_msg);
                
				struct data_t *buf = k_malloc(sizeof(struct data_t));
				if (buf == NULL) {
					return;
				}
				// Copy UART message to dispatcher data
				snprintf(buf->msg, 20, "%s", uart_msg);

				// Put dispatcher data to FIFO buffer
				k_fifo_put(&dispatcher_fifo, buf);
				printk("Fifo data: %s\n", buf->msg);

				// Clear UART receive buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);

				// Clear UART message buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);
			}
		}
		k_msleep(50);
	}
}

static void dispatcher_task(void *, void *, void *)
{
	while (true) {
		// Receive dispatcher data from uart_task fifo
		struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
		char sequence[20];
		memcpy(sequence,rec_item->msg,20);
		k_free(rec_item);

		printk("Dispatcher: %s\n", sequence);

		for (int i=0; i < strlen(sequence); i++) {
			switch (sequence[i]) {
				case 'R':
					k_sem_give(&red_sem);
					k_sem_take(&release_sem, K_FOREVER);
					break;
				case 'Y':
					k_sem_give(&yellow_sem);
					k_sem_take(&release_sem, K_FOREVER);
					break;
				case 'G':
					k_sem_give(&green_sem);
					k_sem_take(&release_sem, K_FOREVER);
					break;
				default:
					printk("Invalid character\n");
					break;
			}
		}
	}
}

static void debug_task(void *, void *, void *) {
	
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
		timing_start();
		timing_t red_start_time = timing_counter_get();

		k_sem_take(&red_sem, K_FOREVER);
		gpio_pin_set_dt(&red,1);
		k_msleep(1000);
		gpio_pin_set_dt(&red,0);
		k_sem_give(&release_sem);

		timing_t red_end_time = timing_counter_get();
		timing_stop();
		uint64_t timing_ns = timing_cycles_to_ns(timing_cycles_get(&red_start_time, &red_end_time));
		printk("Red task: %lld\n", timing_ns / 1000);
	}
}

// Task to handle yellow led
void yellow_led_task(void *, void *, void*) {
	
	printk("Yellow led thread started\n");
	while (true) {
		k_sem_take(&yellow_sem, K_FOREVER);
		gpio_pin_set_dt(&red,1);
		gpio_pin_set_dt(&green,1);
		k_msleep(1000);
		gpio_pin_set_dt(&red,0);
		gpio_pin_set_dt(&green,0);
		k_sem_give(&release_sem);
	}
}

// Task to handle green led
void green_led_task(void *, void *, void*) {
	
	printk("Green led thread started\n");
	while (true) {
		k_sem_take(&green_sem, K_FOREVER);
		gpio_pin_set_dt(&green,1);
		k_msleep(1000);
		gpio_pin_set_dt(&green,0);
		k_sem_give(&release_sem);
	}
}

