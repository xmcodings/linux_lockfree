#define NUMBER_OF_DATA 100
#define NUMBER_OF_THREAD 4

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h> //kmalloc

#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include <linux/delay.h>

#define lflist_for_each(pos, lfhead)				\
	for (pos = lfhead->next; /*exclude first*/	\
	     pos != lfhead;			\
	     pos = pos->next)

struct kernel_node{
	struct list_head list;
	int data;
};

struct node{
	struct node *prev, *next;
	int data;
};

struct lflist_head{
	struct node *start;
	struct node *end;	
};

struct lflist_task{
	struct node *head;
	int data;
}

struct node *head;
struct lflist_head lfhead;

struct list_head klist;
ktime_t start, end;
s64 kernelD_time, improveD_time;

int insertThread(void* index){
	int i;
	int* param = (int*)index;
	start = ktime_get();
	for(i=0; i<NUMBER_OF_DATA/NUMBER_OF_THREAD; i++){
		struct node *new = kmalloc(sizeof(struct kernel_node),GFP_KERNEL);
//		new->data = (*param)*(NUMBER_OF_DATA/NUMBER_OF_THREAD)+i;
//		new->next = NULL;
//		__sync_lock_test_and_set(&new->next, head->next);
//		__sync_lock_test_and_set(&head->next, new);
	}
	end = ktime_get();
	__sync_fetch_and_add(&improveD_time, ktime_to_ns(ktime_sub(end, start)));
	return 0;
}

void lflist_add_tail(void *data){
	int i;
	struct lflist_task *task = (struct lflist_task *)data;
	int insert_value = task->data;
	struct node *head = task->head;

	struct node *new = kmalloc(sizeof(struct node),GFP_KERNEL);
	new->data = i;
	
	struct node *temp;
	int result=0;
	
	// first time
	result = __sync_bool_compare_and_swap(head->next, head, new);
	
	
	do{
		new->prev = head->prev;
		new->next = head;
		head->prev->next = new;	
	}while(!(__sync_bool_compare_and_swap(head->prev, new->next, new)));
	

	if(head->next == head){
		head->next = new;	
	}
	new->prev = head->prev; // new node prev
	head->prev->next = new;
	new->next = head;
	head->prev = new; // end node to new node
	printk("concurrent insert lflist %d \n", new->data);

}

void atomic_insert(){
	struct lflist_task newtask;
}



void compareInsert(void){
	/* kernel data structure */
	struct list_head klist;
	int i;
	INIT_LIST_HEAD(&klist);
	head->prev = head;
	head->next = head;
	improveD_time = 0;

	start = ktime_get();
	for(i=0; i<NUMBER_OF_DATA; i++){
		struct kernel_node *new = kmalloc(sizeof(struct kernel_node),GFP_KERNEL);
		new->data = i;
		list_add(&new->list, &klist);
	}
	end = ktime_get();
	kernelD_time = ktime_to_ns(ktime_sub(end, start));
	printk("---INSERT time---\n Kernel data structure -> %lld ns",(long long)kernelD_time);

	/* thread data structure */
	for(i=0; i<NUMBER_OF_THREAD; i++){
		//kthread_run(&insertThread, (void*)&i,"insertThread");
	}

	msleep(3000); // 쓰레드 다 끝날 때가지 대기
	printk("Imporve data structure -> %lld ns\n",(long long)improveD_time);
	printk("Imporve %lld ns\n\n",(long long)(kernelD_time-improveD_time));
}


void init_node(void){
	int i;
	INIT_LIST_HEAD(&klist);
	kernelD_time = 0;

	// kernel list init 
	start = ktime_get();
	for(i=0; i<NUMBER_OF_DATA; i++){
		struct kernel_node *new = kmalloc(sizeof(struct kernel_node),GFP_KERNEL);
		new->data = i;
		list_add_tail(&new->list, &klist);
		printk("insert klist %d \n", new->data);
	}
	end = ktime_get();
	kernelD_time = ktime_to_ns(ktime_sub(end, start));
	printk("kernel list init time: %lld", (long long)kernelD_time);


	// lflist init
	head = kmalloc(sizeof(struct node),GFP_KERNEL);
	
	// lfhead.start = NULL;
	// lfhead.end = NULL;
	head->prev = head;
	head->next = head;
	start = ktime_get();
	
	for(i=0; i<NUMBER_OF_DATA; i++){
		struct node *new = kmalloc(sizeof(struct node),GFP_KERNEL);
		new->data = i;
		if(head->next == head){
			head->next = new;	
		}
		new->prev = head->prev; // new node prev
		head->prev->next = new;
		new->next = head;
		head->prev = new; // end node to new node
		printk("insert lflist %d \n", new->data);
	}
	end = ktime_get();
	kernelD_time = ktime_to_ns(ktime_sub(end, start));
	printk("head.start data: %d \n", head->next->data);
	printk("lf list init time: %lld", (long long)kernelD_time);
}


void print_lists(void){

	printk("printing kernel list \n");
	struct kernel_node *k_node;
	
	// read example
	list_for_each_entry(k_node, &klist, list){
		printk("klist value: %d\n", k_node->data);
	}

	// lflist read example
	printk("printing lf list \n");
	struct node *current_lfnode;

	current_lfnode = head->next; // from first node
	printk("lflist start data: %d \n", current_lfnode->data);
	int i = 0;
	for(i = 0; i <200; i++){
		
		if(current_lfnode == head){
			printk("list fully traversed \n");
			break;
		}
		printk("lflist_value: %d\n", current_lfnode->data);
		current_lfnode = current_lfnode->next;
	}
	lflist_for_each(current_lfnode, head){
		if(current_lfnode == NULL){
			break;
		}
		printk("lflist_value: %d\n", current_lfnode->data);
	}
}

static int __init list_module_init(void)
{
	printk("Improve Data structure performance! Data %d.\n", NUMBER_OF_DATA);
	//compareInsert();
	//compareDelete();
	//compareTraverse();
	init_node();
	print_lists();

	return 0;
}

static void __exit list_module_cleanup(void)
{
	printk("We are team 8!\n");

	

}

module_init(list_module_init);
module_exit(list_module_cleanup);
MODULE_LICENSE("GPL");
