#ifndef LINKEDLIST_H
#define LINKEDLIST_H


typedef struct Node {
    struct Node *prev;
    void *elm;
    struct Node *next;
} Node;

typedef struct LinkedList {
    Node *head;
    Node *tail;
    int size;
} LinkedList;
        
// Constructor
LinkedList* createList();

// Functions
void insertElement(LinkedList *list, void *newElement);
void* removeElement(LinkedList *list, void *elm1, int (*cmp)(void *elm1, void *elm2));
void printList(LinkedList *list, void(*printElm)(void *elm));

#endif
