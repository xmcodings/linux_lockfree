#define NUMBER_OF_DATA 100000
#define NUMBER_OF_THREAD 10
#define THREAD_WORK 10000

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h> //kmalloc

#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include <linux/delay.h>

#define lflist_for_each(pos, slisthead)				\
	for (pos = slisthead->next; /*exclude first*/	\
	     pos != slisthead;			\
	     pos = pos->next)


struct kernel_node{
	struct list_head list;
	int data;
};

struct node{
	struct node *next, *prev;
	int data;
};

struct list_head klist;

struct node *head;
struct node *tail;

spinlock_t insert_lock;
ktime_t start, end;
ktime_t kstart, kend;
s64 kernelD_time, improveD_time;

int insertThread(void *arg){
	int i;
	start = ktime_get();
	for(i=0; i< THREAD_WORK; i++){
		struct node *new = kmalloc(sizeof(struct kernel_node),GFP_KERNEL);
		new->data = i;
		new->next = NULL;
		new->prev =  __sync_lock_test_and_set(&tail, new);
		__sync_lock_test_and_set(&new->prev->next, new);
	}
	end = ktime_get();
	__sync_fetch_and_add(&improveD_time, ktime_to_ns(ktime_sub(end, start)));
	return 0;
}

static int klistInsertThread (void *data){
	int i;
	kstart = ktime_get();
	for(i=0; i<THREAD_WORK; i++){
		struct kernel_node *new = kmalloc(sizeof(struct kernel_node),GFP_KERNEL);
		new->data = i;
		spin_lock(&insert_lock);
		list_add_tail(&new->list, &klist);
		spin_unlock(&insert_lock);
	}
	kend = ktime_get();
	__sync_fetch_and_add(&kernelD_time, ktime_to_ns(ktime_sub(kend, kstart)));
	do_exit(0);
}

void klistTraverse(void){
	struct kernel_node *k_node;
	list_for_each_entry(k_node, &klist, list){
		if(k_node == NULL){
			break;
		}
		// printk("klist value: %d\n", k_node->data);
		// msleep(1);
	}
}

void lflistTraverse(void){
	struct node *lf_node;
	lflist_for_each(lf_node, head){
		if(lf_node->next == NULL){
			break;
		}
		// printk("lflist value: %d\n", lf_node->data);
		// msleep(1);
	}
}

void compareTraverse(void){

	s64 kernelD_time, improveD_time;

	kstart = ktime_get();
	klistTraverse();
	kend = ktime_get();
	kernelD_time = ktime_to_ns(ktime_sub(kend, kstart));

	start = ktime_get();
	lflistTraverse();
	end = ktime_get();
	improveD_time = ktime_to_ns(ktime_sub(end, start));

	printk("---TRAVERSE time---\nKernel data structure -> %lld ns \n",(long long)kernelD_time);
	printk("Improve data structure -> %lld ns\n",(long long)improveD_time);
	printk("Improve %lld ns\n\n",(long long)(kernelD_time-improveD_time));
}

void compareInsert(void){
	/* kernel data structure */
	int i;
	INIT_LIST_HEAD(&klist);
	head = kmalloc(sizeof(struct kernel_node), GFP_KERNEL);
	head->data = -1;
	tail = head;
	improveD_time = 0;
		
	/* thread data structure */
	for(i=0; i<NUMBER_OF_THREAD; i++)
	{
		kthread_run(&insertThread, NULL,"insertThread");
		kthread_run(&klistInsertThread, NULL,"klistinsertThread");
	}
	msleep(5000);
	printk("---INSERT time---\nKernel data structure -> %lld ns",(long long)kernelD_time);
	printk("Improve data structure -> %lld ns\n",(long long)improveD_time);
	printk("Improve %lld ns\n\n",(long long)(kernelD_time-improveD_time));
}

bool lflist_delete_head(void){
	struct node *current_lfnode;
	struct node *temp;
	current_lfnode = head->next; // first index
	while(current_lfnode != tail){
		temp = current_lfnode;
		if(__sync_bool_compare_and_swap(&head->next, current_lfnode, current_lfnode->next)){
			printk("delete head %d \n", current_lfnode->data);
			return true;
		}
	}
	return false;
}

void lflist_delete(int del){
	struct node *lf_node = head->next;
	struct node *lf_node_prev = head;

	printk("deleting %d\n", del);
	printk("lfnode %d\n", lf_node->data);
	printk("lfnode prev %d\n", lf_node_prev->data);
	
	for(lf_node = head->next; lf_node->next!= NULL;){
		if(lf_node->next == NULL){
			printk("end of list \n");
			break;
		}
		if(lf_node->data == del){
			printk("deleting %d\n", lf_node->data);
			lf_node_prev-> next = lf_node->next;	
			break;
		}
		lf_node_prev = lf_node_prev->next;
		lf_node = lf_node->next;
	}	
}

void compareDelete(int value){
	struct kernel_node *current_node, *temp;
	struct node *cur_node = head;
	improveD_time =0;

	start = ktime_get();
	list_for_each_entry_safe(current_node, temp, &klist, list){
		if(current_node->data == value){
			list_del(&current_node->list);
			kfree(current_node);
		}
	}
	end = ktime_get();
	kernelD_time = ktime_to_ns(ktime_sub(end, start));
	printk("---Delete time---\nKernel data structure -> %lld ns",(long long)kernelD_time);
	
	start = ktime_get();
	do{
		if(cur_node->data == value){
			__sync_lock_test_and_set(&(cur_node->prev->next), cur_node->next);
			__sync_lock_test_and_set(&(cur_node->next->prev), cur_node->prev);
			kfree(cur_node);
			break;
		}	
	}while(__sync_lock_test_and_set(&cur_node, cur_node->next));
        end = ktime_get();
        improveD_time = ktime_to_ns(ktime_sub(end, start));
        printk("Improve data structure -> %lld ns\n",(long long)improveD_time);
        printk("Improve %lld ns\n\n",(long long)(kernelD_time-improveD_time));
}

static int __init list_module_init(void)
{
	printk("Improve Data structure performance! Data %d.\n\n", NUMBER_OF_DATA);
	spin_lock_init(&insert_lock);
	compareInsert();
	compareTraverse();
	compareDelete(8000);
	
	return 0;
}

static void __exit list_module_cleanup(void)
{
	printk("\nWe are team 8!\n");
}

module_init(list_module_init);
module_exit(list_module_cleanup);
MODULE_LICENSE("GPL");
