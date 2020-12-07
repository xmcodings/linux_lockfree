#define NUMBER_OF_DATA 100
#define NUMBER_OF_THREAD 4

#define DFAULT_THREAD 2


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h> //kmalloc
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <stdbool.h>

spinlock_t insert_lock;
spinlock_t delete_lock;

#define lflist_for_each(pos, slisthead)				\
	for (pos = slisthead->next; /*exclude first*/	\
	     pos != slisthead;			\
	     pos = pos->next)

struct kernel_node{
	struct list_head list;
	int data;
};

struct listnode{
	struct listnode *next;
	int data;
};

struct listnode *slisthead;

struct list_head klist;
ktime_t start, end;
ktime_t finaltime;
s64 kernelD_time;
s64 insert_slist_time;
s64 insert_klist_time;
s64 delete_slist_time;
s64 delete_klist_time;


void slist_push(int ins_data){
	struct listnode *new;
	new = kmalloc(sizeof(struct listnode),GFP_KERNEL);
	new->data = ins_data;

	do{
		new->next = slisthead->next;
		printk("push data %d \n", ins_data);
	}while(!__sync_bool_compare_and_swap(&slisthead->next, new->next, new));
	
}

bool slist_delete_tail(void){
	struct listnode *current_lfnode;
	struct listnode *temp;
	current_lfnode = slisthead->next;
	while(current_lfnode){
		temp = current_lfnode;
		if(__sync_bool_compare_and_swap(&slisthead->next, current_lfnode, current_lfnode->next)){
			printk("delete tail %d \n", current_lfnode->data);
			kfree(temp);
			return true;
		}
	}
	return false;
}

static int __synch_slist_pop(void *data){
	start = ktime_get();
	
	int i;
	for(i = 0; i < 25; i++){
		slist_delete_tail();
	}
	end = ktime_get();
	delete_slist_time = ktime_to_ns(ktime_sub(end, start));	
	do_exit(0);
}

static int delete_function(void *data){
	struct kernel_node *current_node;
	struct kernel_node *tmp;
	spin_lock(&delete_lock);
	list_for_each_entry_safe(current_node, tmp, &klist, list){
		list_del(&current_node->list);
		kfree(current_node);
	}
	spin_unlock(&delete_lock);
	end = ktime_get();
	delete_klist_time = ktime_to_ns(ktime_sub(end, start));
}

static int __sync_slist_push(void *ins_data){
	
	start = ktime_get();
	int* ins = (int*)ins_data;
	
	int i;
	for(i = 0; i < 25; i++){
		slist_push(i);
	}
	if(end < ktime_get()){
		end = ktime_get();
		insert_slist_time = ktime_to_ns(ktime_sub(end, start));	
	} 
	do_exit(0);
}

static int list_push(void *data){
	int i;
	
	//start = ktime_get();
	for(i=0; i<25; i++){
		struct kernel_node *new = kmalloc(sizeof(struct kernel_node),GFP_KERNEL);
		new->data = i;
		spin_lock(&insert_lock);
		list_add_tail(&new->list, &klist);
		spin_unlock(&insert_lock);
		// printk("insert klist %d \n", new->data);
	}
	end = ktime_get();
	insert_klist_time = ktime_to_ns(ktime_sub(end, start));	
	// kernelD_time = ktime_to_ns(ktime_sub(end, start));
	do_exit(0);
}


void compareInsert(void){
	/* kernel data structure */
	struct list_head klist;
	int i;
	INIT_LIST_HEAD(&klist);
	s64 improveD_time; 
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

void print_lists(void){

	printk("printing kernel list \n");
	struct kernel_node *k_node;
	
	// // read example
	list_for_each_entry(k_node, &klist, list){
		printk("klist value: %d\n", k_node->data);
	}
	// // lflist read example
	printk("printing lf list \n");
	struct listnode *current_lfnode;

	current_lfnode = slisthead; // from first node
	
	lflist_for_each(current_lfnode, slisthead){
		if(current_lfnode == NULL){
			printk("null detected \n");
			break;
		}
		printk("lflist_value: %d\n", current_lfnode->data);
	}
	msleep(3000);
	/* pop test */

}

void init_node(void){
	
	// kernel list init
	INIT_LIST_HEAD(&klist);
	int i;
	// lflist init
	slisthead = kmalloc(sizeof(struct listnode),GFP_KERNEL);
	
	slisthead->next = slisthead;
	/* thread atomic */

	 start = ktime_get();
	
	for(i=0; i<NUMBER_OF_THREAD; i++){
		kthread_run(__sync_slist_push, NULL, "insert_atomic_function");
		kthread_run(list_push, NULL, "insert_spinlock_function");
	}		
	msleep(3000);

	/* atomic push */

	// start = ktime_get();
	// for(i=0; i<NUMBER_OF_DATA; i++){
	// 	slist_push(i);
	// }
	// end = ktime_get();
	// kernelD_time = ktime_to_ns(ktime_sub(end, start));
	// printk("lf list init time: %lld", (long long)kernelD_time);

	/* iterate */
	print_lists();

	/* delete (pop) whole*/
	start = ktime_get();

	// bool sex;
	// int i = 0;
	// for(i = 0; i <5; i++){
	// 	sex = slist_delete_tail();
	// }

	for(i=0; i<NUMBER_OF_THREAD; i++){
		kthread_run(delete_function, NULL, "insert_atomic_function");
		kthread_run(__synch_slist_pop, NULL, "insert_spinlock_function");
	}	

	// current_lfnode = slisthead; // from first node

	msleep(3000);
}



void contains_thread(void *data){

}

int contains(int elem){

	

}




static int __init list_module_init(void)
{
	spin_lock_init(&insert_lock);
	spin_lock_init(&delete_lock);
	
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
	printk("kernel list init time: %lld", (long long)insert_klist_time);
	printk("lf list init time: %lld", (long long)insert_slist_time);
	printk("kernel list delete time: %lld", (long long)delete_klist_time);
	printk("lf list delete time: %lld", (long long)delete_slist_time);
	printk("We are team 8!\n");

}

module_init(list_module_init);
module_exit(list_module_cleanup);
MODULE_LICENSE("GPL");
