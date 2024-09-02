#include "channel.h"

// Creates a new channel with the provided size and returns it to the caller
// A 0 size indicates an unbuffered channel, whereas a positive size indicates a buffered channel
chan_t* channel_create(size_t size)
{

    if(size < 0){
        return NULL;
    }

    chan_t* channel = (chan_t*)malloc(sizeof(chan_t));
    channel->size = size;
    channel->buffer = buffer_create(size);
    pthread_mutex_init(&(channel->mutex), NULL);
    pthread_mutex_init(&(channel->send_mutex), NULL);
    pthread_mutex_init(&(channel->recv_mutex), NULL);
    sem_init(&channel->send,0,(unsigned int)size);
    sem_init(&channel->receive,0,0);
    channel->flag = 0;
    channel->send_list = list_create();
    channel->recv_list = list_create();
    
    
    return channel;
}

// Writes data to the given channel
// This can be both a blocking call i.e., the function only returns on a successful completion of send (blocking = true), and
// a non-blocking call i.e., the function simply returns if the channel is full (blocking = false)
// In case of the blocking call when the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// WOULDBLOCK if the channel is full and the data was not added to the buffer (non-blocking calls only),
// CLOSED_ERROR if the channel is closed, and
// OTHER_ERROR on encountering any other generic error of any sort
enum chan_status channel_send(chan_t* channel, void* data, bool blocking)
{

    if(blocking == true){ //blocking
        sem_wait(&channel->send); //when channel is full, wait till the channel has space to write the new data
        pthread_mutex_lock(&channel->mutex); //lock protect buffer
        if(channel->flag == 1){ //check if channel closed
            pthread_mutex_unlock(&channel->mutex);
            sem_post(&channel->send); //signal
            return CLOSED_ERROR;
        }
         else{
            buffer_add(data, channel->buffer); //try to write data into given channel
            pthread_mutex_unlock(&channel->mutex); //buffer unlock mutex when done
            sem_post(&channel->receive);
            
           //signal to all semaphores in recv list
            pthread_mutex_lock(&channel->recv_mutex); // protect recv linked list
            list_node_t *current_node = channel->recv_list->head; 
            while(current_node != NULL) {
                sem_post((sem_t*)current_node->data);
                current_node = list_next(current_node);
            }
            pthread_mutex_unlock(&channel->recv_mutex);
            return SUCCESS; //return SUCCESS if data was written
        }
    }

    else if(blocking == false ){ //non-blocking.
        if(sem_trywait(&channel->send) == 0){ //non-blocking wait
            pthread_mutex_lock(&channel->mutex);
            //check of buff is closed
            if(channel->flag == 1) {
                //post. chain reaction
                pthread_mutex_unlock(&channel->mutex);
                sem_post(&channel->send);
                return CLOSED_ERROR;
            }
            else{
                buffer_add(data, channel->buffer); // add data to buffer
                pthread_mutex_unlock(&channel->mutex); // lock protect buffer
                sem_post(&channel->receive); //signal

                //signal to all semaphores in recv list
                pthread_mutex_lock(&channel->recv_mutex); // protect recv linked list
                list_node_t *current_node = channel->recv_list->head;
                while(current_node != NULL){
                    sem_post((sem_t*)current_node->data);
                    current_node = list_next(current_node);
                }
                pthread_mutex_unlock(&channel->recv_mutex);
                return SUCCESS;
            }
        }
        else{
            pthread_mutex_lock(&channel->mutex);
            if(channel->flag == 1){ //check if channel closed
                pthread_mutex_unlock(&channel->mutex); 
                sem_post(&channel->send);
                return CLOSED_ERROR;
            }
            else{
                pthread_mutex_unlock(&channel->mutex);
                return WOULDBLOCK;
            }
        }  
    }   
    return OTHER_ERROR;
    }


// Reads data from the given channel and stores it in the function’s input parameter, data (Note that it is a double pointer).
// This can be both a blocking call i.e., the function only returns on a successful completion of receive (blocking = true), and
// a non-blocking call i.e., the function simply returns if the channel is empty (blocking = false)
// In case of the blocking call when the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// WOULDBLOCK if the channel is empty and nothing was stored in data (non-blocking calls only),
// CLOSED_ERROR if the channel is closed, and
// OTHER_ERROR on encountering any other generic error of any sort
enum chan_status channel_receive(chan_t* channel, void** data, bool blocking)
{
    
    if(blocking == true){ //blocking
        sem_wait(&channel->receive);
        pthread_mutex_lock(&channel->mutex);
        if(channel->flag == 1 ){ //check if channel is closed
            pthread_mutex_unlock(&channel->mutex);
            sem_post(&channel->receive); //signal
            return CLOSED_ERROR;
        }
         else{
            *data = buffer_remove(channel->buffer); //remove data from buffer
            pthread_mutex_unlock(&channel->mutex);
            sem_post(&channel->send);
            
            //signal to all semaphores in send list
            pthread_mutex_lock(&channel->send_mutex);
            list_node_t *current_node = channel->send_list->head;
            while(current_node != NULL){
                sem_post((sem_t*)current_node->data);
                current_node = list_next(current_node);
            }
            pthread_mutex_unlock(&channel->send_mutex);
            return SUCCESS;
        }
    }

    else if(blocking == false){ //non-blocking.
        if(sem_trywait(&channel->receive) == 0){
            //check of buff is full
            pthread_mutex_lock(&channel->mutex);
            if(channel->flag == 1){
                
                pthread_mutex_unlock(&channel->mutex);
                sem_post(&channel->receive);
                return CLOSED_ERROR;
            }

            else{
                
                *data = buffer_remove(channel->buffer);
                pthread_mutex_unlock(&channel->mutex);
                sem_post(&channel->send);
                
               //signal to all semaphores in send list
                pthread_mutex_lock(&channel->send_mutex);
                list_node_t *current_node = channel->send_list->head;
                while(current_node != NULL){
                    sem_post((sem_t*)current_node->data);
                    current_node = list_next(current_node);
                }
                pthread_mutex_unlock(&channel->send_mutex);

                return SUCCESS;
            }
        }
        else{
            pthread_mutex_lock(&channel->mutex);
            if(channel->flag == 1){
                pthread_mutex_unlock(&channel->mutex); 
                sem_post(&channel->receive);
                return CLOSED_ERROR;
            }
            else{
                pthread_mutex_unlock(&channel->mutex);
                return WOULDBLOCK;
            }
        }   
    } 
    return OTHER_ERROR;

}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// OTHER_ERROR in any other error case
enum chan_status channel_close(chan_t* channel)
{

    pthread_mutex_lock(&channel->mutex);

    if(channel->flag == 1){
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    channel->flag = 1;

    sem_post(&channel->send);
    sem_post(&channel->receive);

    pthread_mutex_unlock(&channel->mutex);

    pthread_mutex_lock(&channel->send_mutex);
    list_node_t *current_send_node = channel->send_list->head;
    while(current_send_node != NULL){
        sem_post((sem_t*)current_send_node->data);
        current_send_node = list_next(current_send_node);
    }
    pthread_mutex_unlock(&channel->send_mutex);

    pthread_mutex_lock(&channel->recv_mutex);
    list_node_t *current_recv_node = channel->recv_list->head;
    while(current_recv_node != NULL){
        sem_post((sem_t*)current_recv_node->data);
        current_recv_node = list_next(current_recv_node);
    }
    pthread_mutex_unlock(&channel->recv_mutex);


    return SUCCESS;


}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, andq
// OTHER_ERROR in any other error case
enum chan_status channel_destroy(chan_t* channel)
{
    //check if close was called, if not return error
    if(channel->flag != 1){
        return DESTROY_ERROR;
    }
    //destroy mutexes
    pthread_mutex_destroy(&channel->mutex);
    pthread_mutex_destroy(&channel->send_mutex);
    pthread_mutex_destroy(&channel->recv_mutex);
    
    //destroy semaphores
    sem_destroy(&channel->send);
    sem_destroy(&channel->receive);
    list_destroy(channel->send_list);
    list_destroy(channel->recv_list);

    //free mallocs
    buffer_free(channel->buffer);
    free(channel);

    return SUCCESS;
}

// Takes an array of channels, channel_list, of type select_t and the array length, channel_count, as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum chan_status channel_select(size_t channel_count, select_t* channel_list, size_t* selected_index)
{
    sem_t semaphore;
    int status = 0; //create status of type int to assign to what send/recv functions return and then return that

    sem_init(&semaphore,0,0); //create semaphore

    //insert one semaphore per channel
    for(int i = 0; i < channel_count; i++){

        if(channel_list[i].is_send == true){ //if send, put in send list
            pthread_mutex_lock(&channel_list[i].channel->send_mutex);
            if (list_find(channel_list[i].channel->send_list, &semaphore) == NULL){
                list_insert(channel_list[i].channel->send_list,&semaphore);
            }   
            pthread_mutex_unlock(&channel_list[i].channel->send_mutex);
        }
        else if(channel_list[i].is_send == false){ //if recv, put in recv list
           pthread_mutex_lock(&channel_list[i].channel->recv_mutex);
            if (list_find(channel_list[i].channel->recv_list, &semaphore) == NULL){
                list_insert(channel_list[i].channel->recv_list,&semaphore);
            }
            pthread_mutex_unlock(&channel_list[i].channel->recv_mutex);
        }    
    }


    while(true){
        
        for(size_t i = 0; i < channel_count; i++){ // loop through channel_list
            if(channel_list[i].is_send == true) { //check if its a send operation
                status = channel_send(channel_list[i].channel,channel_list[i].data, 0); //call channel_send as non-blocking call
                if (status != WOULDBLOCK) { //check if function call return WOULDBLOCK error
                    *selected_index = i; //assign selected_index to channel that succeeds
                    //remove sepamphers from all channels
                    for(int j = 0; j < channel_count; j++) { 
                        if(channel_list[j].is_send == true){
                            pthread_mutex_lock(&channel_list[j].channel->send_mutex);
                            list_remove(channel_list[j].channel->send_list, list_find(channel_list[j].channel->send_list, &semaphore));
                            pthread_mutex_unlock(&channel_list[j].channel->send_mutex);
                        }
                        else if(channel_list[j].is_send == false) {
                            pthread_mutex_lock(&channel_list[j].channel->recv_mutex);
                            list_remove(channel_list[j].channel->recv_list, list_find(channel_list[j].channel->recv_list, &semaphore));
                            pthread_mutex_unlock(&channel_list[j].channel->recv_mutex);
                        }
                    }    
                    sem_destroy(&semaphore);
                    return status;
                }

            } else { // recv operation
                status = channel_receive(channel_list[i].channel,&channel_list[i].data, 0); //call channel_receive with non-blocking
                //remove sepamphers from all channels
                if (status != WOULDBLOCK) {  //check if function call return WOULDBLOCK error
                    *selected_index = i; //assign selected_index to channel that succeeds
                    //remove sepamphers from all channels
                    for(int k = 0; k < channel_count; k++){
                        if(channel_list[k].is_send == true){
                            pthread_mutex_lock(&channel_list[k].channel->send_mutex);
                            list_remove(channel_list[k].channel->send_list,list_find(channel_list[k].channel->send_list, &semaphore));
                            pthread_mutex_unlock(&channel_list[k].channel->send_mutex);
                        }
                        else if(channel_list[k].is_send == false){
                            pthread_mutex_lock(&channel_list[k].channel->recv_mutex);
                            list_remove(channel_list[k].channel->recv_list,list_find(channel_list[k].channel->recv_list, &semaphore));
                            pthread_mutex_unlock(&channel_list[k].channel->recv_mutex);
                        } 
                    }    
                    sem_destroy(&semaphore);
                    return status;
                }
            }
        } 
        sem_wait(&semaphore);
    }  
}
