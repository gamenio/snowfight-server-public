#ifndef __REFERENCE_H__
#define __REFERENCE_H__

#include "game/dynamic/LinkedList.h"
#include "debugging/Errors.h"


template <class TO, class FROM> class Reference : public LinkedListElement
{
    private:
        TO* iRefTo;
        FROM* iRefFrom;
    protected:
        // 当FROM对象需要链接到TO对象时被调用
        virtual void buildLink() = 0;

        // 当TO对象需要与FROM对象解除链接时被调用
        virtual void destroyLink() = 0;

    public:
        Reference() { iRefTo = NULL; iRefFrom = NULL; }
        virtual ~Reference() { }

        // 将FROM对象链接到TO对象
		void link(TO* toObj, FROM* fromObj)
		{
			NS_ASSERT(toObj && fromObj);
			if (isValid())
				unlink();

			iRefTo = toObj;
			iRefFrom = fromObj;
			buildLink();
		}

		// 解除TO对象和FROM对象的链接
        void unlink()
        {
			if (!isValid())
				return;

            destroyLink();
            delink();
            iRefTo = NULL;
            iRefFrom = NULL;
        }


        bool isValid() const
        {
            return iRefTo != NULL && iRefFrom != NULL;
        }

        Reference<TO, FROM>       * next()       { return((Reference<TO, FROM>       *) LinkedListElement::next()); }
        Reference<TO, FROM> const* next() const { return((Reference<TO, FROM> const*) LinkedListElement::next()); }
        Reference<TO, FROM>       * prev()       { return((Reference<TO, FROM>       *) LinkedListElement::prev()); }
        Reference<TO, FROM> const* prev() const { return((Reference<TO, FROM> const*) LinkedListElement::prev()); }

        Reference<TO, FROM>       * nocheck_next()       { return((Reference<TO, FROM>       *) LinkedListElement::nocheck_next()); }
        Reference<TO, FROM> const* nocheck_next() const { return((Reference<TO, FROM> const*) LinkedListElement::nocheck_next()); }
        Reference<TO, FROM>       * nocheck_prev()       { return((Reference<TO, FROM>       *) LinkedListElement::nocheck_prev()); }
        Reference<TO, FROM> const* nocheck_prev() const { return((Reference<TO, FROM> const*) LinkedListElement::nocheck_prev()); }

        TO* operator ->() const { return iRefTo; }
        TO* getTarget() const { return iRefTo; }

        FROM* getSource() const { return iRefFrom; }

    private:
        Reference(Reference const&);
        Reference& operator=(Reference const&);
};


#endif // __REFERENCE_H__
