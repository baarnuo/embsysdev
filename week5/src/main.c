/*Pistetavoite 2. Yhden pisteen vaatimukset tehty ja lisätestejä tehty 3 kpl.*/

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <inttypes.h>
#include <zephyr/timing/timing.h>
#include "../parser/TimeParser.h"

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

// Led threads initialization
#define STACKSIZE 500
#define PRIORITY 5
int init_led();
int init_uart();
void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);

// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// Create FIFO buffers
K_FIFO_DEFINE(dispatcher_fifo);
K_FIFO_DEFINE(debug_fifo);

enum data_type {
	DATA_RED,
	DATA_YELLOW,
	DATA_GREEN,
	DATA_DISPATCH
};

// FIFO dispatcher data type
struct data_t {
	void *fifo_reserved;
	char msg[20];
};

// FIFO debug data
struct data_d {
	void *debug_fifo_reserved;
	enum data_type type;
	uint64_t time;
};

K_MUTEX_DEFINE(sequence_time_mutex);
static uint64_t sequence_time = 0;

static void dispatcher_task(void *, void *, void *);
static void uart_task(void *, void *, void *);
static void debug_task(void *, void *, void *);

#define UART_PRIORITY 10
#define DEBUG_PRIORITY 5

K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,UART_PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,UART_PRIORITY,0,0);
K_THREAD_DEFINE(debug_thread,STACKSIZE,debug_task,NULL,NULL,NULL,DEBUG_PRIORITY,0,0);

K_SEM_DEFINE(red_sem, 0, 1);
K_SEM_DEFINE(yellow_sem, 0, 1);
K_SEM_DEFINE(green_sem, 0, 1);
K_SEM_DEFINE(release_sem, 0, 1);

// Timer initializations
struct k_timer timer;
void timer_handler(struct k_timer *timer_id);

void led_handler(struct k_work *work);
K_WORK_DEFINE(led_work, led_handler);

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
				//printk("UART msg: %s\n", uart_msg);
                
				struct data_t *buf = k_malloc(sizeof(struct data_t));
				if (buf == NULL) {
					return;
				}
				// Copy UART message to dispatcher data
				snprintf(buf->msg, 20, "%s", uart_msg);

				// Put dispatcher data to FIFO buffer
				k_fifo_put(&dispatcher_fifo, buf);
				//printk("Fifo data: %s\n", buf->msg);

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

void timer_handler(struct k_timer *timer_id) {
	k_work_submit(&led_work);
}

void led_handler(struct k_work *work) {
	gpio_pin_set_dt(&red,1);
	k_msleep(100);
	gpio_pin_set_dt(&red,0);
	gpio_pin_set_dt(&green,1);
	gpio_pin_set_dt(&red,1);
	k_msleep(100);
	gpio_pin_set_dt(&green,0);
	gpio_pin_set_dt(&red,0);
	gpio_pin_set_dt(&green,1);
	k_msleep(100);
	gpio_pin_set_dt(&green,0);
	gpio_pin_set_dt(&green,1);
	gpio_pin_set_dt(&red,1);
	k_msleep(100);
	gpio_pin_set_dt(&green,0);
	gpio_pin_set_dt(&red,0);
	gpio_pin_set_dt(&red,1);
	k_msleep(100);
	gpio_pin_set_dt(&red,0);
}

static void dispatcher_task(void *, void *, void *) {
	while (true) {
		// Receive dispatcher data from uart_task fifo
		struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);

		printk("Dispatcher: %s\n", rec_item->msg);

		if (rec_item->msg[0] == '0') {
			int ret = time_parse(rec_item->msg);
			if (ret > COMMAND_OK) {
				k_timer_init(&timer, timer_handler, NULL);
				k_timer_start(&timer, K_SECONDS(ret), K_NO_WAIT);
			} else if (ret == TIME_VALUE_ERROR) {
				printk("Time value error\n");
			}
		} else {
			for (int i=0; i < strlen(rec_item->msg); i++) {
				switch (rec_item->msg[i]) {
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

		struct data_d *buf = k_malloc(sizeof(struct data_d));
		if (buf != NULL) {
		    k_mutex_lock(&sequence_time_mutex, K_FOREVER);
		    buf->type = DATA_DISPATCH;
		    buf->time = sequence_time;
		    sequence_time = 0;
		    k_mutex_unlock(&sequence_time_mutex);
		
		    k_fifo_put(&debug_fifo, buf);
		}
	}
}

static void debug_task(void *, void *, void *) {
	
	struct data_d *received;

	while (true) {
		received = k_fifo_get(&debug_fifo, K_FOREVER);

		switch (received->type) {
			case DATA_RED:
				printk("Red exec time: %lld microseconds.\n", received->time / 1000);
				break;

			case DATA_YELLOW:
				printk("Yellow exec time: %lld microseconds.\n", received->time / 1000);
				break;

			case DATA_GREEN:
				printk("Green exec time: %lld microseconds.\n", received->time / 1000);
				break;

			case DATA_DISPATCH:
    			printk("Sequence exec time: %lld microseconds.\n", received->time / 1000);
    			break;
		}
		
		k_free(received);

		k_yield();
	}
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

		struct data_d *buf = k_malloc(sizeof(struct data_d));
		if (buf == NULL) {
			return;
		}

		timing_stop();
		timing_t red_end_time = timing_counter_get();
		uint64_t diff = timing_cycles_to_ns(timing_cycles_get(&red_start_time, &red_end_time));
		
		buf->type = DATA_RED;
		buf->time = diff;
		k_fifo_put(&debug_fifo, buf);

		k_mutex_lock(&sequence_time_mutex, K_FOREVER);
		sequence_time += diff;
		k_mutex_unlock(&sequence_time_mutex);
	}
}

// Task to handle yellow led
void yellow_led_task(void *, void *, void*) {
	
	printk("Yellow led thread started\n");
	while (true) {
		timing_start();
		timing_t yellow_start_time = timing_counter_get();

		k_sem_take(&yellow_sem, K_FOREVER);
		gpio_pin_set_dt(&red,1);
		gpio_pin_set_dt(&green,1);
		k_msleep(1000);
		gpio_pin_set_dt(&red,0);
		gpio_pin_set_dt(&green,0);
		k_sem_give(&release_sem);

		struct data_d *buf = k_malloc(sizeof(struct data_d));
		if (buf == NULL) {
			return;
		}

		timing_stop();
		timing_t yellow_end_time = timing_counter_get();
		uint64_t diff = timing_cycles_to_ns(timing_cycles_get(&yellow_start_time, &yellow_end_time));
		
		buf->type = DATA_YELLOW;
		buf->time = diff;
		k_fifo_put(&debug_fifo, buf);

		k_mutex_lock(&sequence_time_mutex, K_FOREVER);
		sequence_time += diff;
		k_mutex_unlock(&sequence_time_mutex);
	}
}

// Task to handle green led
void green_led_task(void *, void *, void*) {
	
	printk("Green led thread started\n");
	while (true) {
		timing_start();
		timing_t green_start_time = timing_counter_get();

		k_sem_take(&green_sem, K_FOREVER);
		gpio_pin_set_dt(&green,1);
		k_msleep(1000);
		gpio_pin_set_dt(&green,0);
		k_sem_give(&release_sem);

		struct data_d *buf = k_malloc(sizeof(struct data_d));
		if (buf == NULL) {
			return;
		}

		timing_stop();
		timing_t green_end_time = timing_counter_get();
		uint64_t diff = timing_cycles_to_ns(timing_cycles_get(&green_start_time, &green_end_time));
		
		buf->type = DATA_GREEN;
		buf->time = diff;
		k_fifo_put(&debug_fifo, buf);

		k_mutex_lock(&sequence_time_mutex, K_FOREVER);
		sequence_time += diff;
		k_mutex_unlock(&sequence_time_mutex);
	}
}

