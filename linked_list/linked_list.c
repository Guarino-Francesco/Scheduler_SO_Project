#include "linked_list.h"
#include <assert.h>

void List_init(ListHead* head) {
  head->first=0;
  head->last=0;
  head->size=0;
}

// Cerca l'elemento "item" nella lista "head" e se lo trova lo restituisce
ListItem* List_find(ListHead* head, ListItem* item) {

  // Restituisce l'item se lo trova
  for (ListItem* aux=head->first ; aux ; aux=aux->next)
    if (aux==item)
      return item;

  // Altrimenti finito il ciclo ritorna NULL
  return 0;
}

// Inserisce l'elemento "item" di seguito all'elemento "prev" nella lista "head"
ListItem* List_insert(ListHead* head, ListItem* prev, ListItem* item) {
  // Se uno dei due o entrambi non sono nulli ritorna
  if (item->next || item->prev) return 0;

#ifdef _LIST_DEBUG_

  // Controlla che l'elemento non sia gia nella lista
  ListItem* instance=List_find(head, item);
  assert(!instance);

  // Controlla che il "prev" sia nella lista
  if (prev) {
    ListItem* prev_instance=List_find(head, prev);
    assert(prev_instance);
  }

#endif

  // Trova quale deve essere il next di item controllando prev
  ListItem* next=(prev)? prev->next:head->first;

  // Se esiste il prev modifica lui e item di conseguenza
  if (prev) {
    item->prev=prev;
    prev->next=item;
  }

  // Se esiste il next modifica lui e item di conseguenza
  if (next) {
    item->next=next;
    next->prev=item;
  }

  // L'inserimento è in testa (prev = NULL)
  if (!prev) head->first=item;

  // L'inserimento è in coda (next = NULL)
  if (!next) head->last=item;


  head->size++;
  return item;
}

// Scollega l'elemento "item" dalla lista "head"
ListItem* List_detach(ListHead* head, ListItem* item) {

#ifdef _LIST_DEBUG_

  // Controlla che l'elemento sia nella lista
  ListItem* instance=List_find(head, item);
  assert(instance);

#endif

  ListItem* prev=item->prev;
  ListItem* next=item->next;

  // Se esiste il prev modifica il suo next di conseguenza
  if (prev) prev->next=next;

  // Se esiste il next modifica il suo prev di conseguenza
  if (next) next->prev=prev;

  // Se l'elemento era in testa, la nuova testa sara il next
  if (item==head->first) head->first=next;

  // Se l'elemento era in coda, la nuova coda sara il prev
  if (item==head->last) head->last=prev;

  // Elimina i collegamenti con la lista dall'item
  item->next=item->prev=0;

  head->size--;
  return item;
}

// Wrappers
ListItem* List_pushBack(ListHead* head, ListItem* item) {  return List_insert(head, head->last, item); };
ListItem* List_pushFront(ListHead* head, ListItem* item) { return List_insert(head, 0, item);          };
ListItem* List_popFront(ListHead* head) {                  return List_detach(head, head->first);      }
