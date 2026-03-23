#define MAX_PROCESSES 3
typedef struct PCB {
    int pid;
    int start;
    int end;
    int pc;
    int Lscore;
    struct PCB *next;
} PCB;

typedef struct rq {
    PCB *head;
    PCB *tail;
} rq;

int generatePCB();
PCB *createPCB(int start, int end);
void destroyPCB();
int processtable_add(PCB * proc);
int processtable_remove(PCB * proc);

rq *ready_queue_init();
void enqueue(PCB * proc, rq * queue);
PCB *dequeue(rq * queue);
void destroy_rq(rq * queue);

int schedule(char *policy, int proc_count);
void decrement_Lscore(rq* queue);
int Lscore_check(rq *q, PCB *proc);
