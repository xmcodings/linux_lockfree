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

struct listnode{
	struct listnode *next;
	int data;
}

struct node *head;
struct node *slisthead;

struct list_head klist;
ktime_t start, end;
s64 kernelD_time, improveD_time;

void atomic_insert(void){
	struct lflist_task newtask;

}

void slist_push(int ins_data){
	struct listnode *new;
	new = kmalloc(sizeof(struct listnode),GFP_KERNEL);
	new->data = ins_data;

	do{
		new->next = slisthead;
	}while(!__sync_bool_compare_and_swap(&slisthead, new->next, new));
	printk("push data %d \n", ins_data);
}

void insert_after(int elem){


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
	slisthead = kmalloc(sizeof(struct listnode),GFP_KERNEL);
	
	
	start = ktime_get();
	
	for(i=0; i<NUMBER_OF_DATA; i++){
		slist_push(i);
	}
	end = ktime_get();
	kernelD_time = ktime_to_ns(ktime_sub(end, start));
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
	
	lflist_for_each(current_lfnode, head){
		if(current_lfnode == NULL){
			break;
		}
		printk("lflist_value: %d\n", current_lfnode->data);
	}
}

void contains_thread(void *data){

}

int contains(int elem){

	

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
