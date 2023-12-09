# PBS Implementation
PBS can be implemnted in following way:

Step-1: We first declare the variables to hold the priorities and other parameters;


      // Declerations for Priority Based Scheduler
      uint ctime;                  // When was the process created
      uint runtime;                // How long process has been running since last scheduled
      uint stime;                  // How long the process has been sleeping since last scheduled
      uint wtime;                  // How long the process has been waiting in ready to be scheduled
      uint sp;                     // static priority of process
      uint rbi;                    // Recent behavior index
      uint dp;                     // dynamic priority of process
      uint num_scheduled;          // number of times scheduled
Step-2: We make following changes to kernel/proc.calculate

        #ifdef PBS
        printf("PBS\n");
        for (;;)
        {
        // Avoid deadlock by ensuring that devices can interrupt.
        intr_on();
        struct proc *proc_to_sched=0;
        long int dp_proc = __INT64_MAX__;
        for (p = proc; p < &proc[NPROC]; p++)
        {
            acquire(&p->lock);
            if(p->state == RUNNABLE)
            {
            int temp = (int)((3 * p->runtime - p->stime - p->wtime)/(p->runtime + p->stime + p->wtime + 1) * 50);
            if( temp > 0)
            {
                p->rbi = temp;
            }
            else
            {
                p->rbi = 0;
            }
            int sum = p->sp + p->rbi;
            if(sum > 100)
            {
                p->dp = 100;
            }
            else
            {
                p->dp = sum;
            }
            if(dp_proc > p->dp)
            {
                dp_proc = p->dp;
                proc_to_sched = p;
            }
            else if((p->dp == dp_proc))
            {
                if(proc_to_sched->num_scheduled > p->num_scheduled)
                {
                proc_to_sched = p;
                }
                else if(
                proc_to_sched-> num_scheduled == p->num_scheduled
                )
                {
                if(proc_to_sched->ctime > p->ctime)
                {
                    proc_to_sched = p;
                }
                }
            }
            }
            release(&p->lock);
        }
        p = proc_to_sched;
        if(p != 0)
        {
            acquire(&p->lock);
            if ( p->state == RUNNABLE)
            {
            // printf("WHAT\n");
            // Switch to chosen process.  It is the process's job
            // to release its lock and then reacquire it
            // before jumping back to us.
            p->stime = 0;
            p->runtime = 0;
            // p->wtime = 0;
            p->num_scheduled++;
            p->state = RUNNING;
            // p->last_sched=1;
            c->proc = p;
            swtch(&c->context, &p->context);

            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;
            }
            release(&p->lock);
        }
        
Step-3: Then we create the setpriority system call in kernel/sysproc.c

    uint64
    sys_setpriority(void)
    {
      int priority=0;
      int pid=0;
      int return_old=0;
    
      argint(0,&pid);
      argint(1,&priority);
    
      struct proc* p;
      extern struct proc proc[];
    
      for(p=proc ; p < &proc[NPROC] ; p++)
      {
        if(p->pid == pid)
        {
          return_old = p->sp;
          p->sp = priority;
          p->rbi = 25;
          if(priority < return_old)
          {
            yield();
          }
          break;
        }
      }
      return return_old;
    }
    
Step-4: We initialize the system call in syscall.c,syscall.h in kernel.
Step-5: We introduce setpriority.c in which we pass the arguments of call to handler for getting executed in user folder.

    #include "kernel/types.h"
    #include "kernel/stat.h"
    #include "user/user.h"
    
    int main(int argc, char *argv[])
    {
        setpriority(atoi(argv[1]), atoi(argv[2]));
        return 0;
    }
Step-6: At last to test our PBS we schedule the program with schedulertestpbs file with setpriority for IO process - 0,1,2,3 (refer to file in schedulertestpbs.c in user folder).

Here are the analysis:
| Scheduling Algorithm  | Run Time | Wait Time |
|-----------------------|----------|-----------|
| PBS                   | 4        | 88        |
| PBS (CPU-2)           | 2        | 87        |




# Cafe Sim
## Question 1

To calculate the average waiting time, a new variable `w_time` is introduced for each customer. This variable is updated when a barista picks up the order. The code snippet provided calculates the average waiting time by summing the differences between the completion time and the entry time for each order in the simulation. In the given test case, the resulting average waiting time is reported as 2.3 seconds which can be rounded off to 2 seconds. It is emphasized that with an infinite number of baristas, the waiting time tends toward zero, ensuring there is always a free barista for each order.

## Question 2

The report highlights the concept of wasted coffee concerning customer departures. If a customer leaves before their order is completed, and the preparation has already started, the associated coffee is considered wasted. It is clarified that if a customer departs before the coffee enters the preparation phase, no coffees would be deemed wasted. The code calculates and prints the count of wasted coffees at the conclusion of the simulation.

# Ice Cream Parlor Simulation Strategy

## Question 1

**Minimizing Unfulfilled Orders:**

To enhance the fulfillment of orders, a strategy of prioritizing orders with fewer ingredients can be implemented. By monitoring and reserving ingredients for customers with smaller orders, which require fewer ingredients, the aim is to serve a maximum number of complete orders. When ingredient supply becomes limited, a decision is made to reject orders outright if there is no feasible option for ingredient replenishment.

**Ingredient Replenishment Strategy:**

To mitigate incomplete orders, a minimum threshold for topping quantity is established. When a topping falls below this threshold, a replenishment process is initiated to maintain sufficient ingredient levels. If a customer arrives when there is a shortage of a specific topping, they are requested to wait until an adequate quantity of toppings is available to complete their order.

**Unserviced Orders Management:**

Addressing unserviced orders involves optimizing machine timings by considering a coarse parlor-based solution. To make the most of existing resources, a scheduling approach prioritizes orders with the shortest preparation times from the pool of available unprepared customer orders. This allows for the fastest possible service to a maximum number of customers, capitalizing on the availability of machines.

