/**
 * A template wrapper class for a pointer, which can be stored in STL
 * containers. This is principally useful for storing derived types
 * of a base class. This is a reference counted implementation.
 */
#ifndef YARF_REFCOUNTPTR_HPP
#define YARF_REFCOUNTPTR_HPP

#include <cassert>
#include <cstddef>


/**
 * A wrapper for a pointer. This class handles the destruction of the object.
 * Can be constructed from a pointer to an object, which will become owned
 * by this class.
 *
 * If this wrapper is copied or assigned, the contained object is not copied.
 * A count of the number of references to the contained object by PtrWrapper
 * object is maintained and updated whenever this wrapper is copied, assigned
 * or destroyed.
 *
 * Note: Never construct more than one RefCountPtr for an object, otherwise
 * this will really fuck up. Also do not create circular references of these
 * pointers.
 * 
 * @param BaseT The base class type
 */
template <typename BaseT>
class RefCountPtr
{
public:
    /**
     * Constructs a pointer to a object. Assumes ownership of the object
     * referenced by the provided pointer. Default NULL.
     * @param ptr Pointer to the object to be wrapped in a pointer.
     */
    RefCountPtr(BaseT* ptr = NULL) {
        newObject(ptr);
    }

    /**
     * Copy constructor. Doesn't copy the contained object, but maintains
     * a reference count.
     */
    RefCountPtr(const RefCountPtr& oprct) {
        m_pObject = oprct.m_pObject; // The object
        m_nref = oprct.m_nref;   // The ref count
        (*m_nref)++;             // Update
    }

    /**
     * Destructor. Deletes the reference to the object.
     */
    ~RefCountPtr() {
        deleteObj();
    }

    /**
     * Assignment to another instance of this class. Doesn't copy the
     * contained object, but maintains a reference count.
     */
    RefCountPtr& operator=(const RefCountPtr& oprct) {
        if (this != &oprct) // Assigning to itself ??
        {
            // Sanity check: error if the same (apart from null,
            // or if really the same)
            assert(!m_pObject || (m_pObject != oprct.m_pObject) || (m_nref == oprct.m_nref));
            // assert above replaces the following exception:
            //if (m_pObject &&
            //    (m_pObject == oprct.m_pObject) && (m_nref != oprct.m_nref))
            // throw Exception

            deleteObj();

            m_pObject = oprct.m_pObject;
            m_nref = oprct.m_nref;          // Use ref count of copied object
            (*m_nref)++;
        }

        return *this;
    }

    /**
     * Assignment to a pointer takes ownership of the object pointed to.
     * @param ptr Pointer to the object
     * @return Itself
     */
    RefCountPtr& operator=(BaseT* ptr) {
        // Sanity check... but ignore null obv (doh...)
        assert(!m_pObject || (m_pObject != ptr));
        //if (m_pObject && (m_pObject == ptr))
        // throw Exception

        deleteObj();
        newObject(ptr);
        return *this;
    }

    /**
     * Quick test of whether the pointed to value is null
     * @return true if stored pointer is null
     */
    bool isNull() const {
        return m_pObject == NULL;
    }

    /**
     * Overloaded dereference operator to access the contained object.
     * @return A reference to the object pointed to.
     */
    BaseT& operator*() const {
        assert(m_pObject);
        return *m_pObject;
    }

    /**
     * Overloaded member selection operator.
     * @return A pointer (well, kinda) to the object allowing access to the
     * object's members using the -> operator.
     */
    BaseT* operator->() const {
        assert(m_pObject);
        return m_pObject;
    }

    /**
     * Gets the pointer to the object
     * @return A pointer to the object
     */
    BaseT* get() const {
        return m_pObject;
    }

    /**
     * Returns a void pointer which is null if the object pointer is null,
     * otherwise returns a poitner to the object. This function is intended
     * for use in control statements eg. if (pointer), while(pointer), etc.
     * @return pointer to object for use in the test of control statements
     */
    operator void*() const {
        return m_pObject;
    }

    /**
     * Tests if the pointer is null. Same as isNull().
     * @return true if the object pointed to is null
     */
    bool operator!() const {
        return isNull();
    }

    /**
     * Test equality of pointers. Checks whether the pointed to object and the
     * reference count holders are the same (ie. whether both pointers refer
     * to the same object and object count- and yes this means even if both
     * objects are NULL this may still be false if the ref-counts are
     * different)
     * @return true if this pointer is equal to the other
     */
    bool operator==(const RefCountPtr& oprct) {
        return (m_pObject == oprct.m_pObject) &&
            (m_nref == oprct.m_nref);
    }

protected:
    /**
     * Handles destruction of object where necessary
     */
    void deleteObj() {
        assert(*m_nref > 0);

        if (--(*m_nref) == 0)   // Update refs to object
        {
            delete m_pObject;
            delete m_nref;
        }
    }

    /**
     * Handles the addition of a new object
     * @param ptr Pointer to the object
     */
    void newObject(BaseT* ptr) {
        m_pObject = ptr;
        m_nref = new int(1);
    }

    /**
     * Pointer to the object
     */
    BaseT* m_pObject;

    /**
     * Number of references to the object
     */
    int* m_nref;
};

#endif // YARF_REFCOUNTPTR_HPP
