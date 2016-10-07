#include <stdio.h>
#include <stdlib.h>
#include "LinkedList.h"

/**
 * Input:
 *     None
 * Output:
 *     Pointer to a newly created list
 *
 * Allocates the space for a new LinkedList and initializes
 * and returns a pointer to it.
 */
LinkedList* createList() {
    // Allocate the memory for the list
    LinkedList *linkedList = malloc(sizeof(LinkedList));

    // Initialize the list
    linkedList->head = NULL;
    linkedList->tail = NULL;
    linkedList->size = 0;

    // Return the address of this new list
    return linkedList;
}

/**
 * Input:
 *     list - Pointer to a linkedlist
 *     newElement - Pointer to an initialized and allocated element
 * Output:
 *     None
 *
 * Insert the given element into the given list
 */
void insertElement(LinkedList *list, void *newElement) {
    // Allocate Element
    Node *node = malloc(sizeof(Node));

    // Initialize the element
    node->prev = NULL;
    node->elm = newElement;
    node->next = NULL;

    if(list->tail == NULL) {
        // List is empty
        list->head = node;
        list->tail = node;
        node->next = node;
        node->prev = node;
    } else {
        // Add to end of list
        node->prev = list->tail;
        node->next = list->head;
        list->tail->next = node;
        list->head->prev = node;
        list->tail = node;
    }

    list->size += 1;
}

/**
 * Input:
 *     list - Pointer to the linked list
 *     elm1 - Element to be removed
 *     cmp - Compare function for the values in the elements
 * Output:
 *     Pointer to the element that is removed
 *
 * Remove the matching element from the list and return it. Returns null if no element is found.
 */
void* removeElement(LinkedList *list, void *elm1, int (*cmp)(void *elm1, void *elm2)) {
    // Get the head of the list
    Node *head = list->head;
    void *foundElm = NULL;

    // Iterate through the list
    while(head != NULL) {
        if(cmp(elm1, head) == 0) {
            // Adjust the previous and next links to skip the found element
            head->prev->next = head->next;
            head->next->prev = head->prev;

            // Store of the address of the value in the element to be returned
            foundElm = head->elm;
            list->size -= 1;

            // Free the memory allocated to the element
            free(head);

            break;
        }
        
        head = head->next;
    }

    // Return the found element address or null if no match
    return foundElm;
}

/**
 * Input:
 *     list - Pointer to the linked list
 * Output:
 *     pointer to the value of the first node
 *
 * Remove and return the head of the list or NULL if list is empty
 */
void* popElement(LinkedList *list) {
    // Get the head of the list
    Node *head = list->head;

    if(list->size == 0) {
        return NULL;
    } else if(list->size == 1) {
        list->head = NULL;
        list->tail = NULL;
        list->size = 0;

        void *elm = head->elm;

        free(head);

        return elm;
    } else {
        list->head = head->next;
        if(head->next != NULL)
            head->next->prev = list->tail;
        list->tail->prev = head->next;
        list->size -= 1;

        void *elm = head->elm;

        free(head);

        return elm;
    }
}

/**
 * Input:
 *     list - Pointer to the linked list
 *     elm1 - Element to be removed
 *     cmp - Compare function for the values in the elements
 * Output:
 *     Pointer to the element that is removed, NULL if no match
 *
 * Use the given compare function and the given element to find a match if it exists in the list.
 */
void* findElement(LinkedList *list, void *elm1, int (*cmp)(void *elm1, void *elm2)) {
    // Get the head of the list
    Node *head = list->head;
    void *foundElm = NULL;
    int i = list->size;

    // Iterate through the list
    while(head != NULL && i != 0) {
        if(cmp(elm1, head->elm) == 0) {
            foundElm = head->elm;
            break;
        }
        
        head = head->next;
        i--;
    }

    // Return the found element address or null if no match
    return foundElm;
}

/**
 * Input:
 *     list - Pointer to the linked list
 *     printElm - Callback to print the value correctly
 * Output:
 *     None
 *
 * Print the contents of the list
 */
void printList(LinkedList *list, void(*printElm)(void *elm)) {
    // Get the head of the list
    Node *head = list->head;
    int i = list->size;

    // Iterate through the list
    while(head != NULL && i != 0) {
        printElm(head->elm);      
        head = head->next;

        i--;
    }
}

