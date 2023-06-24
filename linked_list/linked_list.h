#pragma once


typedef struct ListItem {
  struct ListItem* prev; // Puntatore all'elemento precedente nella lista (NULL se siamo nel primo elemento)
  struct ListItem* next; // Puntatore all'elemento successivo nella lista (NULL se siamo nell'ultimo elemento)
} ListItem;



typedef struct ListHead {
  ListItem* first; // Puntatore al primo elemento della lista
  ListItem* last;  // Puntatore all'ultimo elemento della lista
  int size;        // Numero di elementi nella lista
} ListHead;



void List_init(ListHead* head);

ListItem* List_find(ListHead* head, ListItem* item);
ListItem* List_insert(ListHead* head, ListItem* previous, ListItem* item);
ListItem* List_detach(ListHead* head, ListItem* item);

ListItem* List_pushBack(ListHead* head, ListItem* item);
ListItem* List_pushFront(ListHead* head, ListItem* item);
ListItem* List_popFront(ListHead* head);
