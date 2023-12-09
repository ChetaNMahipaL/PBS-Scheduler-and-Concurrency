#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include<time.h>
#include<limits.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>


typedef struct variety // store variety of coffee
{
    char name[1024];
    int prep_time;
}coffee;

typedef struct Customer // will store customer requirements in solvable form
{
    int arr_time;
    int custom_no;
    char chosen_coffee[1024];
    int chosen_prep_time;
    int pat_time;
}customer;

// sorting customers to take orders

void merge(customer* arr, int left,int mid, int right)
{
    int len_1 = mid - left + 1;
    int len_2 = right - mid;

    customer left_list[len_1];
    for(int i=0;i<len_1;i++)
        left_list[i]=arr[left+i];

    customer right_list[len_2];
    for(int i=0;i<len_2;i++)
        right_list[i]=arr[mid+1+i];

    int i=0;
    int j=0;
    int k=left;
    
    while(i < len_1 && j < len_2)
    {
        if(left_list[i].arr_time < right_list[j].arr_time)
        {
            arr[k]=left_list[i];
            i++;
        }
        else if(left_list[i].arr_time == right_list[j].arr_time)
        {
            if(left_list[i].custom_no < right_list[j].custom_no)
            {
                arr[k]=left_list[i];  
                i++;              
            }
            else
            {
                arr[k]=right_list[j];
                j++;
            }
        }
        else
        {
            arr[k]=right_list[j];
            j++;            
        }
        k++;
    }
    while(i < len_1)
    {
        arr[k]=left_list[i];  
        i++;     
        k++;      
    }
    while(j < len_2)
    {
        arr[k]=right_list[j];
        j++;
        k++; 
    }
}

void mergesort(int left,int right, customer* arr)
{
    if(left >= right)
        return;

    int mid = left + (right-left)/2;

    mergesort(left,mid,arr);
    mergesort(mid+1,right,arr);

    merge(arr,left,mid,right);
}

// concurrency
sem_t baristas;
sem_t lock;
sem_t c_lock;

// clock variables for synchronization
time_t start_time; // when the cafe starts
time_t rel_time; // for checking seconds passed

// variables for answering questions
int coffee_wasted;
int K;
int B;
int * barist;
// making a global clock
void* global_clock(void* arg)
{
    while(1)
    {
        rel_time = time(NULL) - start_time;
    }
    return NULL;
}

void* update_status (void* arg)
{
    customer* info = (customer*) arg;
    printf("\033[1;37mCustomer %d arrives at %d second(s)\n", info->custom_no,rel_time);
    printf("\033[0m");
    printf("\033[1;33mCustomer %d orders a %s\n", info->custom_no,info->chosen_coffee);
    printf("\033[0m");


    struct timespec tm; // https://www.qnx.com/developers/docs/6.4.1/neutrino/lib_ref/s/sem_timedwait.html
    clock_gettime(CLOCK_REALTIME, &tm);
    tm.tv_sec += info->pat_time;
    // tm.tv_sec += 1;
    
    int brew_time = sem_timedwait(&baristas,&tm);

    if(brew_time == 0)
    {
        int barista_num;    // https://linuxhint.com/sem-getvalue-3-c-function/
        sem_wait(&lock);
        for(int y=0;y<B;y++)
        {
            if(barist[y]==0)
            {
                barista_num = y+1;
                barist[y]=1;
                break;
            }
        }
        sem_post(&lock);
        // sem_getvalue(&baristas,&barista_num);
        // barista_num = B - barista_num;
        sleep(1);
        printf("\033[1;36mBarista %d begins preparing the order of customer %d at %d second(s)\n", barista_num,info->custom_no,rel_time);
        printf("\033[0m");    

        if((rel_time + info->chosen_prep_time) > (info->arr_time+info->pat_time) )
        {
            int cur = rel_time;
            while(rel_time != (info->arr_time+info->pat_time + 1));
            printf("\033[1;31mCustomer %d leaves without their order at %d second(s)\n", info->custom_no,rel_time);
            printf("\033[0m");                
            while(rel_time != (cur+info->chosen_prep_time));
            printf("\033[1;34mBarista %d completes the order of customer %d at %d second(s)\n", barista_num,info->custom_no,rel_time);
            printf("\033[0m");
            sem_wait(&c_lock);
            coffee_wasted++;
            sem_post(&c_lock); 
            sem_wait(&lock);
            barist[barista_num-1]=0; 
            sem_post(&lock);
            sem_post(&baristas);           
        }
        else
        {
            int cur = rel_time;
            while((rel_time - cur) != info->chosen_prep_time);
            printf("\033[1;34mBarista %d completes the order of customer %d at %d second(s)\n", barista_num,info->custom_no,rel_time);
            printf("\033[0m");
            printf("\033[1;32mCustomer %d leaves with their order at %d second(s)\n", info->custom_no,rel_time);
            printf("\033[0m");
            sem_wait(&lock);
            barist[barista_num-1]=0; 
            sem_post(&lock);
            sem_post(&baristas);         
        }
    }
    else
    {
        sleep(1);
        printf("\033[1;31mCustomer %d leaves without their order at %d second(s)\n", info->custom_no,rel_time);
        printf("\033[0m");
    }
    return NULL;

}

int main()
{
    // taking input 
    B = 0;  //Number of Baristas
    K = 0;  //Number of Coffees Available
    int N = 0;  //Number of Customer
    scanf("%d %d %d",&B,&K,&N);
    coffee var_coffee[K];
    for(int i=0;i<K;i++)
    {
        for(int j=0;j<1024;j++)
            var_coffee[i].name[j]='\0';
        
        scanf("%s %d",&var_coffee[i].name, &var_coffee[i].prep_time);
    }
    customer cust_list[N];
    // char temp[1024];
    int index=0;
    for(int i=0;i<N;i++)
    {
        scanf("%d", &cust_list[i].custom_no);
        // index = index - 1;
        for(int j=0;j<1024;j++)
            cust_list[i].chosen_coffee[j]='\0';
        
        scanf("%s %d %d",&cust_list[i].chosen_coffee,&cust_list[i].arr_time,&cust_list[i].pat_time );
        for(int k=0;k<K;k++)
        {
            if(strcmp(cust_list[i].chosen_coffee,var_coffee[k].name)==0)
            {
                cust_list[i].chosen_prep_time=var_coffee[k].prep_time;
                break;
            }
        }
    }
    barist = (int*)calloc(B,sizeof(int));
    // pthread_mutex_init(&lock,NULL);
    sem_init(&lock,0,1);
    sem_init(&c_lock,0,1);

    mergesort(0,N-1,cust_list);
    // for(int i=0; i<N;i++)
    // {
    //     printf("%d %d %d %d\n", cust_list[i].custom_no ,cust_list[i].arr_time, cust_list[i].chosen_prep_time, cust_list[i].pat_time);
    // }

    start_time = time(NULL);

    // function keep running updating time
    pthread_t sync;
    int check_thread = pthread_create(&sync,NULL,&global_clock,NULL);    //https://www.geeksforgeeks.org/thread-functions-in-c-c/
    if(check_thread!=0)
    {
        printf("Error in synchronization\n");
    }
    // for(int i=0;i<10;i++)
    // {
    //     printf("%d\n",rel_time);
    //     sleep(1);
    // }
    coffee_wasted=0;
    sem_init(&baristas,0,B); // intializing B baristas as producers

    // creating thread for each customer
    pthread_t brew[N];
    int count = 0;
    while(count < N)
    {
        if(rel_time >= cust_list[count].arr_time)
        {
            pthread_create(&brew[count],NULL,&update_status, (void* )&cust_list[count]); //https://stackoverflow.com/questions/20196121/passing-struct-to-pthread-as-an-argument
            usleep(50000); // for synchronization of indices
            count++;
        }
    }
    for(int i=0;i<N;i++)
    {
        pthread_join(brew[i],NULL);
    }
    printf("Coffee(s) wasted: %d\n",coffee_wasted);
    return 0;
    
}



