#include <zephyr/kernel.h>
#include "data_fw.h"
#include "sensors/ei_inertial.h"

#define DATA_FW_STACK_SIZE 1024
#define DATA_FW_PRIORITY   5

#define DATA_FW_SAMPLING_FREQUENCY_HZ 50
#define DATA_FW_PERIOD_MS (1000 / DATA_FW_SAMPLING_FREQUENCY_HZ)

#define INERTIAL_AXIS_SAMPLED 6

K_THREAD_STACK_DEFINE(data_fw_stack, DATA_FW_STACK_SIZE);
static struct k_thread data_fw_thread_data;
static k_tid_t data_fw_thread_id;
static bool data_fw_run = false;
static struct k_timer data_fw_timer;
static struct k_sem data_fw_tick_sem;

static void data_fw_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	k_sem_give(&data_fw_tick_sem);
}

static void data_fw_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (data_fw_run) {
		k_sem_take(&data_fw_tick_sem, K_FOREVER);
		if (!data_fw_run) {
			break;
		}

        float inertial_data[INERTIAL_AXIS_SAMPLED] = {0};

        if (ei_inertial_read_data_fwd(inertial_data)) {
            // Data read successfully, you can process or forward the data here
            for (int i = 0; i < INERTIAL_AXIS_SAMPLED; i++) {
                if (i != 0) {
                    printk(",");
                }
                printk("%.3f", inertial_data[i]);
            }
            printk("\n");
        } else {
            printk("Inertial sensor not present or failed to read data\n");
        }

		// Your data forwarding logic here
	}

	data_fw_thread_id = NULL;
}

void data_fw_init(void)
{
	if (data_fw_thread_id != NULL) {
		return;
	}

	k_sem_init(&data_fw_tick_sem, 0, 1);
	k_timer_init(&data_fw_timer, data_fw_timer_handler, NULL);

	data_fw_run = true;
	data_fw_thread_id = k_thread_create(&data_fw_thread_data,
									data_fw_stack,
									K_THREAD_STACK_SIZEOF(data_fw_stack),
									data_fw_task,
									NULL,
									NULL,
									NULL,
									DATA_FW_PRIORITY,
									0,
									K_NO_WAIT);
	k_thread_name_set(data_fw_thread_id, "data_fw");
	k_timer_start(&data_fw_timer, K_MSEC(DATA_FW_PERIOD_MS), K_MSEC(DATA_FW_PERIOD_MS));
}

void data_fw_deinit(void)
{
	k_tid_t thread_id = data_fw_thread_id;

	if (thread_id == NULL) {
		return;
	}

	data_fw_run = false;
	k_timer_stop(&data_fw_timer);
	k_sem_give(&data_fw_tick_sem);

	if (k_current_get() != thread_id) {
		(void)k_thread_join(&data_fw_thread_data, K_SECONDS(1));
		data_fw_thread_id = NULL;
	}
}
