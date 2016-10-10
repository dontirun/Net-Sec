#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

typedef struct Node {
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
void* popElement(LinkedList *list);
void* removeElement(LinkedList *list, void *elm1, int (*cmp)(void *elm1, void *elm2));
void* findElement(LinkedList *list, void *elm1, int (*cmp)(void *elm1, void *elm2));
void printList(LinkedList *list, void(*printElm)(void *elm));

#endif
