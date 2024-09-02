#include "linked_list.h"


// Creates and returns a new list
list_t* list_create()
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    list_t *linked_list = (list_t*)malloc(sizeof(list_t));
    linked_list->count = 0;
    linked_list->head = NULL;
    return linked_list;
}

// Destroys a list
void list_destroy(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */

    list_node_t *current_node = list->head;

    while (current_node != NULL) {
        list_node_t *node_to_free = current_node;
        current_node = current_node->next;
        free(node_to_free);
    }
    free(list);

}

// Returns beginning of the list
list_node_t* list_begin(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return list->head;
}

// Returns next element in the list
list_node_t* list_next(list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return node->next;

}

// Returns data in the given list node
void* list_data(list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return node->data;
}

// Returns the number of elements in the list
size_t list_count(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return list->count;
}

// Finds the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t* list_find(list_t* list, void* data)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    
    list_node_t *current_node = list->head;

    while(current_node != NULL) {
        if(current_node->data == data){
            
            return current_node;
        }
        current_node = current_node->next;
        
    }
    
    return NULL;
}

// Inserts a new node in the list with the given data
void list_insert(list_t* list, void* data)
{
    
    if(list == NULL){
        return;
    }
    list_node_t* new_node = (list_node_t*)malloc(sizeof(list_node_t)); 
    new_node->data = data;
    
    list_node_t *first_node = list->head;

    if(new_node == NULL){
        return;
    }

    if (first_node == NULL) {
        new_node->prev = NULL;
        new_node->next = NULL;
        list->head = new_node; // Update the head of the list
    } else {
        first_node->prev = new_node;
        new_node->prev = NULL;
        new_node->next = first_node;
        list->head = new_node; // Update the head of the list
    }

    list->count += 1;

}

// Removes a node from the list and frees the node resources
void list_remove(list_t* list, list_node_t* node)
{
    
    if (list == NULL || node == NULL) {
            // Handle null pointer cases
            return;
        }
    //case1: if node is in the center
    if(node->prev != NULL && node->next != NULL){
        node->prev->next = node->next;
        node->next->prev = node->prev; 
        list->count-=1;
        free(node);
       
    }

    //case2: if node is at the beginin
    else if(node->prev == NULL && node->next != NULL){
        node->next->prev = NULL;
        list->head = node->next;
        list->count-=1;
        free(node);
        
    }
    
    //case3: if node is at the end
    else if(node->prev != NULL && node->next == NULL){
        node->prev->next = NULL;
        list->count-=1;
        free(node);
        
    }
    //only node in list
    else if(node->prev == NULL && node->next == NULL){
        list->head = NULL;
        list->count-=1;
        free(node);
        
    }

}

// Executes a function for each element in the list
void list_foreach(list_t* list, void (*func)(void* data))
{

    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
}
