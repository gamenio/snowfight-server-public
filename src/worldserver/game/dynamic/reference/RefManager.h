#ifndef __REF_MANAGER_H__
#define __REF_MANAGER_H__


#include "game/dynamic/LinkedList.h"
#include "Reference.h"

template <class TO, class FROM> class RefManager : public LinkedListHead
{
    public:
        typedef LinkedListHead::Iterator< Reference<TO, FROM> > iterator;
        RefManager() { }
        virtual ~RefManager() { clearReferences(); }

        Reference<TO, FROM>* getFirst() { return ((Reference<TO, FROM>*) LinkedListHead::getFirst()); }
        Reference<TO, FROM> const* getFirst() const { return ((Reference<TO, FROM> const*) LinkedListHead::getFirst()); }
        Reference<TO, FROM>* getLast() { return ((Reference<TO, FROM>*) LinkedListHead::getLast()); }
        Reference<TO, FROM> const* getLast() const { return ((Reference<TO, FROM> const*) LinkedListHead::getLast()); }

		void insertFirst(Reference<TO, FROM>* ref)
		{
			LinkedListHead::insertFirst(ref);
		}

		void insertLast(Reference<TO, FROM>* ref)
		{
			LinkedListHead::insertLast(ref);
		}

        iterator begin() { return iterator(getFirst()); }
        iterator end() { return iterator(NULL); }
        iterator rbegin() { return iterator(getLast()); }
        iterator rend() { return iterator(NULL); }

        void clearReferences()
        {
			Reference<TO, FROM>* ref;
            while ((ref = getFirst()) != NULL)
            {
				ref->unlink();
            }
        }
};


#endif // __REF_MANAGER_H__

