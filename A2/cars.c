#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "traffic.h"

extern struct intersection isection;

/**
 * Populate the car lists by parsing a file where each line has
 * the following structure:
 *
 * <id> <in_direction> <out_direction>
 *
 * Each car is added to the list that corresponds with 
 * its in_direction
 * 
 * Note: this also updates 'inc' on each of the lanes
 */
void parse_schedule(char *file_name) {
    int id;
    struct car *cur_car;
    struct lane *cur_lane;
    enum direction in_dir, out_dir;
    FILE *f = fopen(file_name, "r");

    /* parse file */
    while (fscanf(f, "%d %d %d", &id, (int*)&in_dir, (int*)&out_dir) == 3) {

        /* construct car */
        cur_car = malloc(sizeof(struct car));
        cur_car->id = id;
        cur_car->in_dir = in_dir;
        cur_car->out_dir = out_dir;

        /* append new car to head of corresponding list */
        cur_lane = &isection.lanes[in_dir];
        cur_car->next = cur_lane->in_cars;
        cur_lane->in_cars = cur_car;
        cur_lane->inc++;
    }

    fclose(f);
}

/**
 * TODO: Fill in this function
 *
 * Do all of the work required to prepare the intersection
 * before any cars start coming
 * 
 */
void init_intersection() {
    for (int i=0; i<4;i++){
        //Initialize the quardrant locks
        isection.quad[i] = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        //pthread_mutex_unlock(isection.quad[i]);
        isection.lanes[i].lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        //Initialize the condition variables
        isection.lanes[i].producer_cv=(pthread_cond_t)PTHREAD_COND_INITIALIZER;
        isection.lanes[i].consumer_cv=(pthread_cond_t)PTHREAD_COND_INITIALIZER;
        //Initliaze in and out buffer
        isection.lanes[i].in_cars = NULL;
        isection.lanes[i].out_cars = NULL;
        //Prepare the counts
        isection.lanes[i].inc = 0; 
        isection.lanes[i].passed = 0;
        //Prepare the circular buffer
        isection.lanes[i].buffer = (struct car**)malloc(
            sizeof(struct car*)*LANE_LENGTH);
        isection.lanes[i].head = 0;  //Index of the head element
        isection.lanes[i].tail = 0;
        isection.lanes[i].capacity = LANE_LENGTH;
        isection.lanes[i].in_buf = 0;
    }
}

/**
 * TODO: Fill in this function
 *
 * Populates the corresponding lane with cars as room becomes
 * available. Ensure to notify the cross thread as new cars are
 * added to the lane.
 * 
 */
void *car_arrive(void *arg) {
    struct lane *l = arg;
    
    /*We are essentially put cars in the buffer from in_cars*/
   
      
    struct car * curr = l->in_cars;  //This is the pointer of current car 
    								
    struct car *cur_car; //For deep copy
    while (curr!=NULL){ //For every car in the linked list
        
        pthread_mutex_lock(&l->lock); 

        while(l->in_buf==l->capacity) { //If the buffer is full
            pthread_cond_wait(&l->producer_cv, &l->lock);
            
            
            //Wait on the producer cv and release the lane lock
        }
        
        /*If the buffer is not full, We add car to the buffer tail*/
        cur_car = (struct car*)malloc(sizeof(struct car));
        cur_car->id = curr->id;
        cur_car->in_dir = curr->in_dir;
        cur_car->out_dir = curr -> out_dir;
        cur_car->next = NULL;
        
        
        l->buffer[l->tail] = cur_car;
        l->tail++; l->in_buf++;
        //Go deep copy and put in the buffer
        if (l->tail == l->capacity) {//if the tail is out of bound
            l->tail = 0;   //We rotate it to the head 
        }
        curr = curr->next;

        pthread_cond_signal(&l->consumer_cv);
        pthread_mutex_unlock(&l->lock);
       
         /*Then we signal the crossing thread that there is a new car */
        /*Release the lock for the lane */
      
    }
    
    
    
    
    
    


    return NULL;
}

/**
 * TODO: Fill in this function
 *
 * Moves cars from a single lane across the intersection. Cars
 * crossing the intersection must abide the rules of the road
 * and cross along the correct path. Ensure to notify the
 * arrival thread as room becomes available in the lane.
 *
 * Note: After crossing the intersection the car should be added
 * to the out_cars list of the lane that corresponds to the car's
 * out_dir. Do not free the cars!
 *
 * 
 * Note: For testing purposes, each car which gets to cross the 
 * intersection should print the following three numbers on a 
 * new line, separated by spaces:
 *  - the car's 'in' direction, 'out' direction, and id.
 * 
 * You may add other print statements, but in the end, please 
 * make sure to clear any prints other than the one specified above, 
 * before submitting your final code. 
 */
void *car_cross(void *arg) {
    struct lane *l = arg;

   
    struct car* front_car; 
    int* car_path;
    
    
    
    while(l->passed != l->inc) {//There are still cars to pass
    	//NOT in the critical section 
        //printf("Cross: head=%d, tail=%d \n", l->head, l->tail);
        pthread_mutex_lock(&l->lock); 
        /*Grab the lock for the lane*/
        while(l->in_buf==0) { //If the buffer is empty
            /*First we go thourgh the buffer to get cars out*/
            pthread_cond_wait(&l->consumer_cv, &l->lock);
        }
        /*if the buffer is not empty, we get a car out of the buffer*/
        front_car = l->buffer[l->head]; l->head++; l->in_buf--;
        if (l->head ==l->capacity){ l->head = 0;}
        /*If the head goes out of cound, set it to zero*/
        struct car * car_arr = (struct car*)malloc(
                            sizeof(struct car));
                        car_arr->id = front_car->id;
                        car_arr->in_dir = front_car->in_dir;
                        car_arr->out_dir = front_car->out_dir;
                        car_arr->next = NULL;
        //Deep copy to avoid circular buffer change pointer                 
        pthread_mutex_unlock(&l->lock); 
        //following infomation access are not in the critical section 
        //This is done to allow U-Turn, dangerous for MESA 
        printf("%d %d %d\n",front_car->in_dir, front_car->out_dir, front_car->id);
        //Print for test
        /*Get the path of the car*/
        car_path = compute_path(front_car->in_dir, front_car->out_dir);
        //printf("Car Path for ID:%d:%d->%d->%d->%d \n", front_car->id,
        //car_path[0],car_path[1],car_path[2],car_path[3] );
        for (int i=0;i<4;i++){

            if (car_path[i]>0){ //Step the quads on the path by locking
                 pthread_mutex_lock(&isection.quad[car_path[i]]);
                 //Step the quads on the path by locking
                 if (i==3) {//Reach the end and the destination
                    int dest = car_path[i];
                    
                   
                    pthread_mutex_lock(&isection.lanes[dest].lock);
                    //Grab the lock to write to out_cars  
                    //Add the item on the out_cars
                    
                  if (isection.lanes[dest].out_cars == NULL){
                        isection.lanes[dest].out_cars = car_arr;

                    }
                    else { 
                         struct car* tail =  isection.lanes[dest].out_cars;
                         while(tail->next!=NULL){
                            tail = tail->next;
                         }

                        tail->next = car_arr; }
                    
                    pthread_mutex_unlock(&isection.lanes[dest].lock); 
                    //After the car passed, unlock all the acquired lock 
                    for (int j=3;j>=0;j--){
                        if (car_path[j]>0){
                           
                            pthread_mutex_unlock(&isection.quad[car_path[j]]);
                        }
                    }
                    l->passed++;
                    
                    
                    //Done the job 
                    
                }
                 else if (car_path[i+1]<0){
                    //Reached the destination
                    
                    int dest = car_path[i];
                    //Grab the lock to write to out_cars 
                    
                    pthread_mutex_lock(&isection.lanes[dest].lock); 

                    //Add the item on the out_cars
                    //Preaprea to add the car     
                    if (isection.lanes[dest].out_cars == NULL){
                        isection.lanes[dest].out_cars = car_arr;
                        
                    }
                    else {
                         struct car* tail =  isection.lanes[dest].out_cars;
                         while(tail->next!=NULL){
                            tail = tail->next;
                         }

                        tail->next = car_arr; }
                    
                    
                    pthread_mutex_unlock(&isection.lanes[dest].lock); 
                    
                    for (int j=3;j>=0;j--){
                        if (car_path[j]>0){
                           
                            pthread_mutex_unlock(&isection.quad[car_path[j]]);
                        }
                    }
                    l->passed++;
                 

                   
              }//if car_path[i] > 0
                

            }
        }//After one car is handled 
        free(car_path);
        pthread_cond_signal(&l->producer_cv); 
		
       
   	   
        
    }//End of while loop
    free(l->buffer);
    struct car* prev = l->in_cars;
    struct car* curr = l->in_cars;
    while (curr!=NULL){
        prev = curr; 
        curr = curr->next;
        free(prev);
    }
    
    
    return NULL;
}

/**
 * TODO: Fill in this function
 *
 * Given a car's in_dir and out_dir return a sorted 
 * list of the quadrants the car will pass through.
 * 
 */
int *compute_path(enum direction in_dir, enum direction out_dir) {
    int* path = (int*)malloc(sizeof(int) * 4);
    /*Use -1 to mark the end */
    if (in_dir == NORTH){
        if(out_dir == NORTH){
            path[0] = 1; path[1] = 0; path[2] = -1; path[3] = -1;
        }
        else if(out_dir == SOUTH){
            path[0] = 1; path[1] = 2; path[2] = -1; path[3] = -1; 
        }
        else if(out_dir == WEST){
            path[0] = 1; path[1] = -1; path[2] = -1; path[3] = -1; 
        }
        else if(out_dir == EAST){
            path[0] = 1; path[1] = 2; path[2] = 3; path[3] = -1; 
        }
        else { path = NULL; /*This is a safety*/}
        return path;
    }

    else if (in_dir == SOUTH){
        if(out_dir == NORTH){
            path[0] = 3; path[1] = 0; path[2] = -1; path[3] = -1;
        }
        else if(out_dir == SOUTH){
            path[0] = 3; path[1] = 0; path[2] = -1; path[3] = -1; 
        }
        else if(out_dir == WEST){
            path[0] = 3; path[1] = 0; path[2] = 1; path[3] = -1; 
        }
        else if(out_dir == EAST){
            path[0] = 3; path[1] = -1; path[2] = -1; path[3] = -1; 
        }
        else { path = NULL; }
        return path;
    }

    else if (in_dir == WEST){
        if(out_dir == NORTH){
            path[0] = 2; path[1] = 3; path[2] = 0; path[3] = -1;
        }
        else if(out_dir == SOUTH){
            path[0] = 2; path[1] = -1; path[2] = -1; path[3] = -1; 
        }
        else if(out_dir == WEST){
            path[0] = 2; path[1] = 1; path[2] = -1; path[3] = -1; 
        }
        else if(out_dir == EAST){
            path[0] = 2; path[1] = 3; path[2] = -1; path[3] = -1; 
        }
        else { path = NULL; }
        return path;
    }

    else if (in_dir == EAST){
        if(out_dir ==NORTH){
            path[0] = 0; path[1] = -1; path[2] = -1; path[3] = -1;
        }
        else if(out_dir == SOUTH){
            path[0] = 0; path[1] = 1; path[2] = 2; path[3] = -1; 
        }
        else if(out_dir == WEST){
            path[0] = 0; path[1] = 1; path[2] = -1; path[3] = -1; 
        }
        else if(out_dir == EAST){
            path[0] = 0; path[1] = 3; path[2] = -1; path[3] = -1; 
        }
        else { path = NULL; }
        return path;
    }

    else {return NULL;}
    
}