#ifndef __TYPE_CONTAINER_VISITOR_H__
#define __TYPE_CONTAINER_VISITOR_H__

#include "TypeContainer.h"

// TypeGridContainer
// Visitor helpers
template<typename VISITOR>
void visitorHelper(VISITOR&, ContainerGrid<TypeNull>&){ }

template<typename VISITOR, typename OBJECT>
void visitorHelper(VISITOR& v, ContainerGrid<OBJECT>& c)
{
	v.visit(c.element);
}

template<typename VISITOR, typename H, typename T>
void visitorHelper(VISITOR& v, ContainerGrid<TypeList<H, T>>& c)
{
	visitorHelper(v, c.elements);
	visitorHelper(v, c.tailElements);
}

template<typename VISITOR, typename OBJECT_TYPES>
void visitorHelper(VISITOR& v, TypeGridContainer<OBJECT_TYPES>& c)
{
	visitorHelper(v, c.getElements());
}

// TypeUnorderedSetContainer
// Visitor helpers
template<typename VISITOR>
void visitorHelper(VISITOR&, ContainerUnorderedSet<TypeNull>&) { }

template<typename VISITOR, typename OBJECT>
void visitorHelper(VISITOR& v, ContainerUnorderedSet<OBJECT>& c)
{
	v.visit(c.element);
}

template<typename VISITOR, typename H, typename T>
void visitorHelper(VISITOR& v, ContainerUnorderedSet<TypeList<H, T>>& c)
{
	visitorHelper(v, c.elements);
	visitorHelper(v, c.tailElements);
}

template<typename VISITOR, typename OBJECT_TYPES>
void visitorHelper(VISITOR& v, TypeUnorderedSetContainer<OBJECT_TYPES>& c)
{
	visitorHelper(v, c.getElements());
}

// TypeContainerVisitor
template<typename VISITOR, typename TYPE_CONTAINER>
class TypeContainerVisitor
{
public:
	TypeContainerVisitor(VISITOR& v) :m_visitor(v) {}

	void visit(TYPE_CONTAINER& c)
	{
		visitorHelper(m_visitor, c);
	}

	void visit(TYPE_CONTAINER& c) const
	{
		visitorHelper(m_visitor, c);
	}

private:
	VISITOR& m_visitor;
};



#endif //__TYPE_CONTAINER_VISITOR_H__
