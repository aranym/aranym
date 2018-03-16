/*
 * Double linked list implementation
 * Copyright (c) 2006 windom authors (see AUTHORS file)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Taken from:
 * WinDom: a high level GEM library
 * Copyright (c) 1997-2006 windom authors (see AUTHORS file)
 *
 */

#ifndef __WINDOM_LIST__
#define __WINDOM_LIST__

/* __BEGIN_DECLS */

/** @addtogroup List 
 *  @{
 */

/** element used to link structures each others
 *
 *  This data shall be set at the very first place of the structure
 *  if you want all the macro to work correctly.
 */
typedef struct Linkable {
	struct Linkable *next;  /**< pointer to previous structure in the list */
	struct Linkable *prev;  /**< pointer to next structure in the list */
} LINKABLE;


/** definition of a list of linked structures 
 *
 *  This is the starting point of the list of your elements.
 *  This data point to both the first and the last element 
 *  of the list.
 * 
 */
typedef struct {
	LINKABLE head;  /**< first element of the list: this is a dummy element */
	LINKABLE tail;  /**< last element of the list: this is a dummy element */
} LIST;


/** this is a zero-ed LIST data 
 *
 *  Warning: such list won't be reconized as an empty list.
 *  You should call listInit() to properly initialise/zero/empty a list.
 */
#define listEmptyListInitializer() {{NULL,NULL},{NULL,NULL}}

/** Initialisation of an empty LIST
 *
 *  @param list the LIST data to initialise
 *
 *  Initialises a list : \a list will be set as an
 *  empty list.
 */
static inline void listInit( LIST *list ) {
	list->head.next = &list->tail;
	list->head.prev = (LINKABLE*)0L;
	list->tail.next = (LINKABLE*)0L;
	list->tail.prev = &list->head;
}

/** Insert the \a entry before \a iter
 *
 *  @param iter is an element of the chained list
 *  @param entry is a new element to insert in the list
 *
 */
static inline void listInsert( LINKABLE *iter, LINKABLE *entry ) {
	entry->next = iter;
	entry->prev = iter->prev;
	iter->prev->next = entry;
	iter->prev = entry;
}

/** Insert the \a entry after \a iter
 *
 *  @param iter is an element of the chained list
 *  @param entry is a new element to insert in the list
 *
 */
static inline void listAppend( LINKABLE *iter, LINKABLE *entry ) {
	entry->next = iter->next;
	entry->prev = iter;
	iter->next->prev = entry;
	iter->next = entry;
}

/** remove \a iter from the list
 *
 *  @param iter is the element of the chained list to remove
 *
 */
static inline LINKABLE *listRemove( LINKABLE *iter ) {
	iter->prev->next = iter->next;
	iter->next->prev = iter->prev;
	return iter;
}

LIST *createList( void );
LIST *listSplice( LIST *list, LINKABLE *first, LINKABLE *pastLast );

/** usefull macro to parse all elements of a list. 
 *
 *  @param type is the type of the structure (to cast the \a i data)
 *  @param i is the data to use in for loop. This data should be of the 
 *         \a type type.
 *  @param list is the list to parse.
 *
 *  This macro is based on a real \c for instruction, so you may leave
 *  this loop by using the \c break instruction.
 *
 *  Very important: the first data of the \a type structure must be a LINKABLE data.
 *  
 *  @par Exemple of use:
 *  
@code 
typedef struct {
	LINKABLE link;
	int other_datas;
} ELT;


LIST *list;
ELT *e;
(...)
listForEach( ELT*, e, list) {
	int found;
	if (e->other_datas)
		found = do_stuff(e);
	else
		found = do_other_stuff(e);
	if (found)
		break;
}
@endcode
 *
 */

#define listForEach( type, i, list )	\
	if ( list ) for( i=(type)(list)->head.next; \
		((LINKABLE*)i) != &(list)->tail; \
		i=(type)((LINKABLE*)i)->next )

/** return the very first LINKABLE object of the list (before the first element linked) */
#define listRewind( list )	&((list)->head)

/** return the very last LINKABLE object of the list (after the last element linked) */
#define listEnd( list )		&((list)->tail)

/** return the next element on list. If the end of the list is reached (that is if the next
 *  element is the very last LINKABLE object of the list) */
#define listNext( iter )	((!((LINKABLE*)iter)->next->next) ? NULL : (((LINKABLE*)iter)->next))

/** return the previous element on list. If the begin of the list is reached (that is if the previous
 *  element is the very first LINKABLE object of the list) */
#define listPrev( iter )	((!((LINKABLE*)iter)->prev->prev) ? NULL : (((LINKABLE*)iter)->prev))

/** return if the list is empty or not */
#define listIsEmpty(list)	( ((list)->head.next) == (&(list)->tail))

/** return the first linked element of the list (the LINKABLE object right after listRewind),
 *  or NULL if the list is empty */
#define listFirst(list)		( ((list)->head.next) == (&(list)->tail) ? NULL : ((list)->head.next))
#define listLast(list)		( ((list)->tail.prev) == (&(list)->head) ? NULL : ((list)->tail.prev))

typedef struct LinkablePtr {
	LINKABLE	iter;
	void		*value;
} LINKABLEPTR;


/** @} */

/* __END_DECLS */

#endif /* __WINDOM_LIST__ */
