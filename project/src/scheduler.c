#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "scheduler.h"
#include "shell.h"
#include "shellmemory.h"

PCB *processtable[MAX_PROCESSES]; 

//PCB functions

int generate_pid(){
    static int PID_COUNTER = 0;
    PID_COUNTER++;
    return PID_COUNTER;
}

PCB *createPCB(int start, int end){

    // if(start == -1){
    //   PCB *p = calloc(1, sizeof(*p));
    //   return p;
    // }

    PCB *p = malloc(sizeof(*p));
    
    if (p == NULL) {
      //TODO handle allocation failure
    }

    p -> pid = generate_pid();
    p -> start = start;
    p -> end = end;
    p -> pc = start;
    p -> next = NULL;
    p -> Lscore = end - start;

    return p;
}

void destroy_PCB(){
// TODO
}

int processtable_add(PCB *proc){

  for(int i = 0; i < MAX_PROCESSES; i++){
    if(processtable[i] == NULL){
      processtable[i] = proc;
      return 0;
    }
  }
  //program should never reached this point because of the argument count check in interpreter.c
  printf("Too many scheduled scripts");
  return 1;
}

int processtable_remove(PCB *proc){
  if (!proc) return 1;
  
  for(int i = 0; i < MAX_PROCESSES; i++){
    if (processtable[i] != NULL && processtable[i]->pid == proc->pid){
      free(processtable[i]);
      processtable[i] = NULL;
      return 0;
    }
  }

  printf("processtable_remove: process not found\n");
  return 1;
}

//Ready queue functions

rq *run_queue_init(){
  rq *r = calloc(1,sizeof(*r));
  return r;
}

void destroy_rq(rq* queue){
  //TODO
}

void enqueue(PCB* proc, rq* queue){
  proc->next = NULL;
  //if queue is empty put the process at the head
  if (queue -> head == NULL){
    queue -> head = proc;
    queue -> tail = proc;
    return;
  }

  queue -> tail -> next = proc;
  queue -> tail = proc;
}

PCB *dequeue(rq* queue){
  if (queue -> head == NULL){
    //TODO handle empty queue
    return NULL;
  }else{
    PCB *temp = queue -> head;
    queue -> head = temp -> next;

    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    temp->next = NULL;
    return temp;
  }
}

void enqueue_SJF(PCB* proc, rq* queue){
  int proc_size = proc -> Lscore;
  

  //put the process at the head if queue is empty
  if(queue -> head == NULL){
    queue -> head = proc;
    queue -> tail = proc;
    return;
  }
  
  PCB *curr = queue -> head;
  PCB *prev = NULL;
  while(curr != NULL){
    int curr_size = curr -> Lscore;

    if(curr_size > proc_size){// if our process is shorter than the current one

      if(prev == NULL) {
        queue -> head = proc; //update the head of the queue
      }else{
        prev -> next = proc;
      }
      proc -> next = curr;
      return;
      //no need to update curr -> next or queue -> tail

    }else if (curr_size == proc_size){
      if(curr -> pid > proc -> pid){ //if our process came before the current one

        if(prev == NULL) {
          queue -> head = proc; //update the head of the queue
        }else{
          prev -> next = proc;
        }
        proc -> next = curr;
        return;
        //no need to update curr -> next or queue -> tail
 
       //if both proc in the queue are the same size (edge case for when ...)
      }else if(curr -> next && curr -> next -> Lscore == curr_size && curr -> next -> pid < proc -> pid){ 
        
        proc -> next = curr -> next -> next;
        curr -> next -> next = proc;
        if(curr -> next == queue -> tail) queue -> tail = proc;
        return;

      }else{ //anyother case we insert after curr
        proc -> next = curr -> next;
        curr -> next = proc;
        if(curr == queue->tail) queue->tail = proc;
        return;
      }

    // if our process is larger than curr and we are checking the last one
    }else if (curr_size < proc_size && curr -> next == NULL){
    
      curr -> next = proc;
      queue -> tail = proc;
      return;
    
    //if our process is larger than the curr and we're not checking the last element in the queue
    }else{
      prev = curr;
      curr = curr -> next;
    }
  }
}

int schedule(char *policy, int proc_count){
  rq *rq = run_queue_init();
  if (!rq) {
    printf("Error: Failed to initialize run queue\n");
    return 1;
  }
  int errCode = 0;
  int i;

 //--------------FCFS--------------------------------------------------------
  if(strcmp(policy, "FCFS") == 0){
    // First, enqueue all processes
    // printf("Enqueueing %d processes\n", proc_count);
    for(i = 0; i < proc_count; i++){
      if(processtable[i]) {
        // printf("Enqueueing process %d (PID: %d)\n", i, processtable[i]->pid);
        enqueue(processtable[i], rq);
      }
    }

    // Then execute them
    PCB *proc;
    while((proc = dequeue(rq)) != NULL) {
      // printf("Executing process PID: %d\n", proc->pid);
      int curr = proc->start;
      int end = proc->end;

      //execute the code
      while(curr < end){
        char *line = code_get_line(curr);
        if (!line) {
            printf("Error: Failed to get line at position %d\n", curr);
            return 1;
        }
        // printf("%s\n", line);
        errCode = parseInput(line);
        //if(errCode == 1){return 1;}
        curr++;
      }
      
      // After process completes, clean it up
      reset_code_mem(proc->start, end);
      
      // Find and clear this process from the process table
      for(int j = 0; j < MAX_PROCESSES; j++) {
        if(processtable[j] && processtable[j]->pid == proc->pid) {
          processtable[j] = NULL;
          break;
        }
      }
      free(proc);
    }
  }
 //--------------------SJF-------------------------------------------------------------
  if(strcmp(policy, "SJF") == 0){
    for(i = 0; i < proc_count; i++){
      enqueue_SJF(processtable[i], rq);
    }

    PCB *proc;
    for(i = 0; i < proc_count; i++){
      proc = dequeue(rq);
      int curr = proc->start;
      int end = proc->end;

      //execute the code
      while(curr < end){
        char *line = code_get_line(curr);
        errCode = parseInput(line);
        if(errCode == 1){return 1;}
        curr++;
      }
      
      reset_code_mem(proc->start, end);
      
      // Find and clear this process from the process table
      for(int j = 0; j < MAX_PROCESSES; j++) {
        if(processtable[j] && processtable[j]->pid == proc->pid) {
          processtable[j] = NULL;
          break;
        }
      }
      free(proc);
    }
  }
 //--------------------RR-------------------------------------------------------------
  if(strcmp(policy, "RR") == 0){
    //first enqueue in FCFS mode
    for(i = 0; i < proc_count; i++){
      if(processtable[i]) {
        enqueue(processtable[i], rq);
      }
    }

    PCB *proc = dequeue(rq);
    int count = 0;
    int curr, end;

    while(proc){
      curr = proc->pc;
      end = proc->end;

      //if the program ends
      if(curr >= end){
        reset_code_mem(proc->start, end);

        // Find and clear this process from the process table
        for(int j = 0; j < MAX_PROCESSES; j++) {
          if(processtable[j] && processtable[j]->pid == proc->pid) {
            processtable[j] = NULL;
            break;
          }
        }
        free(proc);
        proc = NULL;

        proc = dequeue(rq);
        count = 0;
        continue;
      }

      //change program
      if (count == 2){
        enqueue(proc, rq);
        proc = dequeue(rq);
        count = 0;
        continue;
      }

      char *line = code_get_line(curr);
      errCode = parseInput(line);
      if(errCode == 1){return 1;}
      proc->pc++;
      count++;
    }
  }

 //--------------------AGING-------------------------------------------------------------
  if(strcmp(policy, "AGING") == 0){

    //first enqueue based on length
    for(i = 0; i < proc_count; i++){
      if(processtable[i]) {
        enqueue_SJF(processtable[i], rq);
      }
    }

    PCB *proc = dequeue(rq);
    int curr, end;
    while(proc){

      curr = proc -> pc;
      end = proc -> end;

      //if program ends
      if(curr >= end){
        reset_code_mem(proc->start, end);

        // Find and clear this process from the process table
        for(int j = 0; j < MAX_PROCESSES; j++) {
          if(processtable[j] && processtable[j]->pid == proc->pid) {
            processtable[j] = NULL;
            break;
          }
        }
        //free(proc);
        proc = NULL;

        proc = dequeue(rq);
        continue;
      }

      char *line = code_get_line(curr);
      //printf("%s at line %d\n", line, curr);
      errCode = parseInput(line);
      //printf("%d\n", errCode);
      if(errCode == 1){return 1;}
      proc->pc++;
      decrement_Lscore(rq);

      if(Lscore_check(rq, proc)){
        enqueue_SJF(proc, rq);
        proc = dequeue(rq);
      }
    }
  } // end AGING

 //--------------------RR30-------------------------------------------------------------
  if(strcmp(policy, "RR30") == 0){
    //first enqueue in FCFS mode
    for(i = 0; i < proc_count; i++){
      if(processtable[i]) {
        enqueue(processtable[i], rq);
      }
    }

    PCB *proc = dequeue(rq);
    int count = 0;
    int curr, end;

    while(proc){
      curr = proc->pc;
      end = proc->end;

      //if the program ends
      if(curr >= end){
        reset_code_mem(proc->start, end);

        // Find and clear this process from the process table
        for(int j = 0; j < MAX_PROCESSES; j++) {
          if(processtable[j] && processtable[j]->pid == proc->pid) {
            processtable[j] = NULL;
            break;
          }
        }
        free(proc);
        proc = NULL;

        proc = dequeue(rq);
        count = 0;
        continue;
      }

      //change program
      if (count == 30){
        enqueue(proc, rq);
        proc = dequeue(rq);
        count = 0;
        continue;
      }

      char *line = code_get_line(curr);
      //printf("%s at line %d", line, curr);
      errCode = parseInput(line);
      if(errCode == 1){return 1;}
      proc->pc++;
      count++;
    }
  }
    
  return errCode;
}

void decrement_Lscore(rq *queue){
  PCB *proc = queue -> head;
  while(proc){
    if (proc ->Lscore > 0) proc -> Lscore--;
    proc = proc ->next;
  }
}

int Lscore_check(rq *q, PCB *proc){
  PCB *curr = q ->head;
  if(proc -> Lscore == 0) return 0;
  while(curr){
    if(curr -> Lscore < proc -> Lscore) return 1;
    curr = curr -> next;
  }

  return 0;
}