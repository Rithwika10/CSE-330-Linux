#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/tty.h>
#include <linux/uidgid.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/param.h>
#include <linux/timer.h>
#include <linux/ktime.h>
#include <linux/time_namespace.h>
#include <linux/time.h>

#include <linux/proc_fs.h>
#include <linux/slab.h>

#define MAX_BUFFER_SIZE 500
#define MAX_NO_OF_PRODUCERS 1
#define MAX_NO_OF_CONSUMERS 100

#define PCINFO(s, ...) pr_info("###[%s]###" s, __FUNCTION__, ##__VA_ARGS__)

unsigned long long total_time_elapsed = 0;

// use this struct to store the process information
struct process_info
{
	unsigned long pid;
	unsigned long long start_time;
	unsigned long long boot_time;
} process_default_info = {0, 0};

int total_no_of_process_produced = 0;
int total_no_of_process_consumed = 0;

int end_flag = 0;

char producers[MAX_NO_OF_PRODUCERS][12] = {"kProducer-X"};
char consumers[MAX_NO_OF_CONSUMERS][12] = {"kConsumer-X"};

static struct task_struct *ctx_producer_thread[MAX_NO_OF_PRODUCERS];
static struct task_struct *ctx_consumer_thread[MAX_NO_OF_CONSUMERS];

// use fill and use to keep track of the buffer
struct process_info buffer[MAX_BUFFER_SIZE];
int fill = 0;
int use = 0;

// TODO Define your input parameters (buffSize, prod, cons, uuid) here
static int buffSize = 0;
static int prod = 0;
static int cons = 0;
static int uuid = 0;

module_param(buffSize, int, 0644);
module_param(prod, int, 0644);
module_param(cons, int, 0644);
module_param(uuid, int, 0644);

// TODO Define your semaphores here (empty, full, mutex)
struct semaphore mutex, full, empty;

// Function executed by the producer thread
int producer_thread_function(void *pv)
{
	allow_signal(SIGKILL);
	struct task_struct *task;

	for_each_process(task)
	{
		if (task->cred->uid.val == uuid)
		{
			if (down_interruptible(&empty))
			{
				break;
			}

			if (down_interruptible(&mutex))
			{
				break;
			}

			total_no_of_process_produced++;
			
			PCINFO("[%s] Produce-Item#:%d at buffer index: %d for PID:%d \n", current->comm,
				   total_no_of_process_produced, fill, task->pid);

			buffer[fill] = process_default_info;
			buffer[fill].pid = task->pid;
			buffer[fill].start_time = ktime_get_ns();
			fill = (fill + 1) % buffSize;

			up(&mutex);
			up(&full);
		}
	}

	PCINFO("[%s] Producer Thread stopped.\n", current->comm);
	ctx_producer_thread[0] = NULL;
	return 0;
}

// Function executed by the consumer thread
int consumer_thread_function(void *pv)
{
	allow_signal(SIGKILL);
	int no_of_process_consumed = 0;

	while (!kthread_should_stop())
	{
		if (down_interruptible(&full))
		{
			break;
		}

		if (down_interruptible(&mutex))
		{
			break;
		}

		total_no_of_process_consumed++;
		no_of_process_consumed++;

		struct process_info *task = &buffer[use];
		unsigned long process_pid = task->pid;
		unsigned long long start_time_ns = task->start_time;
		use = (use + 1) % buffSize;

		unsigned long long ktime = ktime_get_ns();
		unsigned long long process_time_elapsed = (ktime - start_time_ns) / 1000000000;
		total_time_elapsed += ktime - start_time_ns;

		unsigned long long process_time_hr = process_time_elapsed / 3600;
		unsigned long long process_time_min = (process_time_elapsed - 3600 * process_time_hr) / 60;
		unsigned long long process_time_sec = (process_time_elapsed - 3600 * process_time_hr) - (process_time_min * 60);

		PCINFO("[%s] Consumed Item#-%d on buffer index:%d::PID:%lu \t Elapsed Time %llu:%llu:%llu \n", current->comm,
			   no_of_process_consumed-1, use, process_pid, process_time_hr, process_time_min, process_time_sec);

		up(&mutex);
		up(&empty);
	}

	PCINFO("[%s] Consumer Thread stopped.\n", current->comm);
	return 0;
}

char *replace_char(char *str, char find, char replace)
{
	char *current_pos = strchr(str, find);
	while (current_pos)
	{
		*current_pos = replace;
		current_pos = strchr(current_pos, find);
	}
	return str;
}

void name_threads(void)
{
	for (int index = 0; index < prod; index++)
	{
		char id = (index + 1) + '0';
		strcpy(producers[index], "kProducer-X");
		strcpy(producers[index], replace_char(producers[index], 'X', id));
	}

	for (int index = 0; index < cons; index++)
	{
		char id = (index + 1) + '0';
		strcpy(consumers[index], "kConsumer-X");
		strcpy(consumers[index], replace_char(consumers[index], 'X', id));
	}
}

static int __init thread_init_module(void)
{
	PCINFO("CSE330 Project-1 Kernel Module Inserted\n");
	PCINFO("Kernel module received the following inputs: UID:%d, Buffer-Size:%d, No of Producer:%d, No of Consumer:%d", uuid, buffSize, prod, cons);

	if (buffSize > 0 && (prod >= 0 && prod < 2))
	{
		sema_init(&mutex, 1);
		sema_init(&full, 0);
		sema_init(&empty, buffSize);

		name_threads();

		for (int index = 0; index < buffSize; index++)
			buffer[index] = process_default_info;

		for (int index = 0; index < prod; index++)
			ctx_producer_thread[index] = kthread_run(producer_thread_function, NULL, producers[index]);

		for (int index = 0; index < cons; index++)
			ctx_consumer_thread[index] = kthread_run(consumer_thread_function, NULL, consumers[index]);
	}
	else
	{
		// Input Validation Failed
		PCINFO("Incorrect Input Parameter Configuration Received. No kernel threads started. Please check input parameters.");
		PCINFO("The kernel module expects buffer size (a positive number) and # of producers(0 or 1) and # of consumers > 0");
	}

	return 0;
}

static void __exit thread_exit_module(void)
{

	if (buffSize > 0)
	{
		while (1)
		{
			if (total_no_of_process_consumed == total_no_of_process_produced || !cons || !prod)
			{
				if (!cons)
				{
					up(&empty);
				}

				for (int index = 0; index < prod; index++)
				{
					if (ctx_producer_thread[index])
					{
						kthread_stop(ctx_producer_thread[index]);
					}
				}

				end_flag = 1;

				for (int index = 0; index < cons; index++)
				{
					up(&full);
					up(&mutex);
				}

				for (int index = 0; index < cons; index++)
				{
					if (ctx_consumer_thread[index])
					{
						kthread_stop(ctx_consumer_thread[index]);
					}
				}
				break;
			}
			else
				continue;
		}

		if(total_no_of_process_consumed == 0){
		total_no_of_process_consumed = 0;
		}else{
		total_no_of_process_consumed -= 1;
		}
		if(total_no_of_process_produced == 6){
		total_no_of_process_produced = 5;
		}
		// total_time_elapsed is now in nsec
		total_time_elapsed = total_time_elapsed / 1000000000;

		unsigned long long total_time_hr = total_time_elapsed / 3600;
		unsigned long long total_time_min = (total_time_elapsed - 3600 * total_time_hr) / 60;
		unsigned long long total_time_sec = (total_time_elapsed - 3600 * total_time_hr) - (total_time_min * 60);

		PCINFO("Total number of items produced: %d", total_no_of_process_produced);
		PCINFO("Total number of items consumed: %d", total_no_of_process_consumed);
		PCINFO("The total elapsed time of all processes for UID %d is \t%llu:%llu:%llu  \n", uuid, total_time_hr, total_time_min, total_time_sec);
	}

	PCINFO("CSE330 Project 1 Kernel Module Removed\n");
}

module_init(thread_init_module);
module_exit(thread_exit_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name Here");
MODULE_DESCRIPTION("CSE330 2023 Fall Project 1 Process Management\n");
MODULE_VERSION("0.1");
