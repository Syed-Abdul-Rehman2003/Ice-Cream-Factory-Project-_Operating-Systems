#include<linux/module.h>//for defining work in kernel modules
#include<linux/kernel.h>//printk()
#include<linux/semaphore.h>//for sem functions
#include<linux/delay.h>//delaying execution in kernel
#include<linux/kthread.h>//creating anad managing kthreads
#include<linux/sched.h>//functiosn and DS for shceduling process and threads
#include<linux/time.h>//for timers
#include<linux/timer.h>//for managing and creating timers

#define MAXORDER 15 //Max order allowed is 15 only for this project
#define NOOFORDER 4 //no of order max 4 only for this project
//necessary functions
int Cust(void *id);
int StartManufacture(void *);
int Manufacture(void *id);
//semaphores
static struct semaphore WL;//semaphores for waiting line
static struct semaphore OC;//order in process
static struct semaphore boil, sugar, flavor, cone, freez, wrap;//semaphpres for manufacture process	
static struct semaphore CW;
int value=0;		//not all order have been processed.
int CurrentOrder;		//global for getting knowledge of current order processing.
int Num[MAXORDER];
int const details[NOOFORDER] = {3,1,2,4};//direct input data, integer array of order details for 4 orders customer will arrive in this order
static struct task_struct *childThreads[MAXORDER];//line declares a static array of task_struct pointers named childThreads, with a size of MAXORDER, holding pointer to a newly created child kernel thread.
static struct task_struct *tid[MAXORDER];//declares another static array of task_struct pointers named tid, also with a size of MAXORDER holding pointer to a newly created kernel thread,responsible for orders
//main function to be called when sys_icecream call is made 
asmlinkage long sys_icecream(void) 
{
	//checking if the number of orders within th limit
	//if greater then terminate
	if(NOOFORDER>MAXORDER)
	{
		printk(KERN_INFO "The number of orders in greater than the MAX capacity. Exiting...\n");
		return 0;
	} 
	printk(KERN_INFO "\t---DATA\n");
	printk(KERN_INFO "numOfOrders: %d\n", NOOFORDER);
	for (i=0;i<NOOFORDER;i++) printk(KERN_INFO "%d \n", details[i]);//print the value of ciurrent element 
	printk(KERN_INFO "-----------\n");
	printk(KERN_INFO "\t\t---Solution for Ice cream factory---\n");
	for (i= 0;i<MAXORDER;++i)
	{
		Num[i]=i+1;
	}
	//initiazlie the semaphores
	sema_init(&WL,  NOOFORDER);	// waiting line has initial value = NOOFORDER
	sema_init(&OC,1);	//1 ordering machine.
	sema_init(&boil,3);	//number of boilers = 3.
	sema_init(&sugar,2);	//number of sugar = 2.
	sema_init(&flavor,2);	//2 flavour machines.
	sema_init(&cone,2);	//number of cone = 2.
	sema_init(&freez,3);	//number of freezing = 3.
	sema_init(&wrap,2);	//number of wrapping = 2.
	sema_init(&CW,0);//initialize the cust wait with 0
	//create threads for orders
	char T[8]="thread1";
	for (i=0;i<NOOFORDER; ++i)
	{
		tid[i] = kthread_create(Cust,(void*)&Num[i],T);//storing ids created byy customer fun
		if((tid[i]))
		{
			//printk(KERN_INFO "---Debug--- tid[] not NULL. calling wake_up_process");
			wake_up_process(tid[i]);
			//printk(KERN_INFO "---Debug--- wake_up_process called on tid[].\n");
		}
	}
	msleep(3000);//after creating cust. thread main thread wiats for 3 sec
	//to allow all threads to be created
	value = 1;//all order processed 
	printk(KERN_INFO "Program ended\n");//program ended
	return 0;
}
//func representing kernel thread for simulating cust order. 
int Cust(void *id)
{
	int no = *(int *)id;
	printk(KERN_INFO "The order number %d has been received.\n", no);
	msleep(10);//order recieved sleeps for 10/1000s
	down(&WL);//acquiring this sem for limiting orders in waiting line	
	printk(KERN_INFO "The order number %d is in waiting area now.\n", no);
	down(&OC);//acquiring ordercounter for limiting no of orders processed at a time
	up(&WL);
	printk(KERN_INFO "The order number %d has entered the counter\n", no);
	CurrentOrder=no;
	//printk(KERN_INFO "---Debug--- currentOrderRunning: %d\n", currentOrderRunning);
	StartManufacture((void *)0);//simulating the manufactire process
	printk(KERN_INFO "The order #%d has been processed. Order leaving.\n", no);
	up(&OC);//releases the ordercounter sem and prints mess
	do_exit(0);//terminating the thrread 
	return 0;
}
///creating child threads to manufacture the  ice cream cones 
int StartManufacture(void *nothing)
{
	int temp[details[CurrentOrder-1]];//hold the no of incecrema in each order
	int i;
	for (i=0;i<details[CurrentOrder-1]; ++i)
	{
		temp[i]=i+1;
	}
	//create a separate child thread for each cone 
	for (i=0;i<details[CurrentOrder-1]; ++i)
	{
		childThreads[i] = kthread_create(Manufacture,(void*)&temp[i],"thread");
		//if kthread creates successfuy a child thread then wake up for starting the execution
		if((childThreads[i]))
		{
			// printk(KERN_INFO "---Debug--- Inside childThreads[][]. wakeup calling.\n");
			wake_up_process(childThreads[i]);
			// printk(KERN_INFO "---Debug--- The wake_up_process was called on childThreads[][].\n");
		}
	}
	msleep(500); //and sleep for 500ms
	return 0;
}
//similar functionality but for manufacturing
int Manufacture(void *id)
{
	//printk(KERN_INFO "---Debug--- inside manufacture function.\n");
	int no = *(int *)id;
	down(&boil);//acquire semaphore for boiling
	printk(KERN_INFO "The order number # %d's ice cream # %d is in boiling stage.\n", CurrentOrder, no);
	msleep(15);//sleep
	up(&boil);//release
	down(&sugar);//accquire for sugaring
	printk(KERN_INFO "The order number # %d's ice cream # %d is in sugar stage.\n", CurrentOrder, no);
	msleep(10);//sleep
	up(&sugar);//then release
	down(&flavor);//acquire for flavoring
	printk(KERN_INFO "The order number # %d's ice cream # %d is in flavor stage.\n", CurrentOrder, no);
	msleep(10);//sleep
	up(&flavor);//release
	down(&cone);//accquiring for coning
	printk(KERN_INFO "The order number # %d's ice cream # %d is in coning stage.\n", CurrentOrder, no);
	msleep(10);//sleep 
	up(&cone);//release
	down(&freez);//acquire for freezing
	printk(KERN_INFO "The order number # %d's ice cream # %d is in freezing stage.\n", CurrentOrder, no);
	msleep(15);//sleep
	up(&freez);//release
	down(&wrap);//acquire for wrapping
	printk(KERN_INFO "The order number # %d's ice cream # %d is in wrapping stage.\n", CurrentOrder, no);
	msleep(10);//sleep
	up(&wrap);////release for freezing
	printk(KERN_INFO "The order number # %d's ice cream # %d is finished/made.\n", CurrentOrder, no);
	do_exit(0);
	return 0;
}

