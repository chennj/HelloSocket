#ifndef _AUTOPTR_HPP_
#define _AUTOPTR_HPP_

template<class C>
class AutoPtr
{
private:
	C* _ptr;
public:
	AutoPtr() :_ptr(0)
	{

	}

	AutoPtr(C* ptr) :_ptr(ptr)
	{

	}

	AutoPtr(C* ptr, bool shared) :_ptr(ptr)
	{
		if (shared && _ptr)_ptr->clone();
	}

	AutoPtr(const AutoPtr& ptr) :_ptr(ptr._ptr)
	{
		if (_ptr)_ptr->clone();
	}

	template<class Other>
	AutoPtr(const AutoPtr<Other>& ptr) : _ptr(const_cast<Other*>(ptr.get()))
	{
		if (_ptr)_ptr->clone();
	}

	~AutoPtr()
	{
		if (_ptr)
		{
			_ptr->release();
			delete _ptr;
		}
	}

public:
	AutoPtr& assign(C* ptr)
	{
		if (_ptr != ptr)
		{
			if (_ptr)_ptr->release();
			_ptr = ptr;
		}
		return *this;
	}

	AutoPtr& assign(C* ptr, bool shared)
	{
		if (_ptr != ptr)
		{
			if (_ptr)_ptr->release();
			_ptr = ptr;
			if (shared && _ptr)_ptr->clone();
		}
		return *this;
	}

	AutoPtr& assign(const AutoPtr& ptr)
	{
		if (&ptr != this)
		{
			if (_ptr)_ptr->release();
			_ptr = &ptr;
			if (_ptr)_ptr->clone();
		}
		return *this;
	}

	template<class Other>
	AutoPtr& assign(const AutoPtr<Other>& ptr)
	{
		if (ptr.get() != _ptr)
		{
			if (_ptr)_ptr->release();
			_ptr = const_cast<Other*>(ptr.get());
			if (_ptr)_ptr->clone();
		}
		return *this;
	}

	void swap(AutoPtr& ptr)
	{
		std::swap(_ptr, ptr._ptr);
	}

	C* clone()
	{
		if (_ptr)_ptr->clone();
		return _ptr;
	}

	bool isNull() const
	{
		return _ptr == 0;
	}

	C* get()
	{
		return _ptr;
	}

	const C* get() const
	{
		return _ptr;
	}

public:
#if defined(_MSC_VER)
#if _MSC_VER>=1300
	/**
	*	Return an AutoPtr containing NULL if the case fail
	*	Example:
	*		assume class Child : public Parent
	*		AutoPtr<Parent> parent(new Child());
	*		AutoPtr<Child> child = parent.cast<Child>();
	*		assert(!child.get())
	*/
	template<class Other>
	AutoPtr<Other> cast() const
	{
		Other* pOther = dynamic_cast<Other*>(_ptr);
		return AutoPtr(pOther, true);
	}

	/**
	*	1)
	*	const_cast can be used to change non-const class members inside a const member function.
	*	Consider the following code snippet. Inside const member function fun(),
	*	¡®this¡¯ is treated by the compiler as ¡®const student* const this¡¯, i.e.
	*	¡®this¡¯ is a constant pointer to a constant object,
	*	thus compiler doesn¡¯t allow to change the data members through ¡®this¡¯ pointer.
	*	const_cast changes the type of ¡®this¡¯ pointer to ¡®student* const this¡¯.
	*	Example:
	*	--------
	*	class A{
	*		int data;
	*		// A const function that changes roll with the help of const_cast
	*		void fun() const{
	*			( const_cast <A*> (this) )->data = 5; //success
	*			this->data = 5;//fail
	*		}
	*	}
	*	2)
	*	const_cast can be used to pass const data to a function that doesn¡¯t receive const.
	*	For example, in the following program fun() receives a normal pointer,
	*	but a pointer to a const can be passed with the help of const_cast.
	*	Example:
	*	--------
	*	int fun(int* ptr) { return (*ptr + 10); }
	*	const int val = 10;
	*	const int *ptr = &val;
	*	int *ptr1 = const_cast <int *>(ptr);
	*	cout << fun(ptr1); //success
	*/
	template<class Other>
	AutoPtr<Other> unsafeCast() const
	{
		Other* pOther = const_cast<Other*>(_ptr);
		return AutoPtr(pOther, true);
	}
#endif
#else
	/**
	*	Return an AutoPtr containing NULL if the case fail
	*	Example:
	*		assume class Child : public Parent
	*		AutoPtr<Parent> parent(new Child());
	*		AutoPtr<Child> child = parent.cast<Child>();
	*		assert(!child.get())
	*/
	template<class Other>
	AutoPtr<Other> cast() const
	{
		Other* pOther = dynamic_cast<Other*>(_ptr);
		return AutoPtr(pOther, true);
	}

	/**
	*	1)
	*	const_cast can be used to change non-const class members inside a const member function.
	*	Consider the following code snippet. Inside const member function fun(),
	*	¡®this¡¯ is treated by the compiler as ¡®const student* const this¡¯, i.e.
	*	¡®this¡¯ is a constant pointer to a constant object,
	*	thus compiler doesn¡¯t allow to change the data members through ¡®this¡¯ pointer.
	*	const_cast changes the type of ¡®this¡¯ pointer to ¡®student* const this¡¯.
	*	Example:
	*	--------
	*	class A{
	*		int data;
	*		// A const function that changes roll with the help of const_cast
	*		void fun() const{
	*			( const_cast <A*> (this) )->data = 5; //success
	*			this->data = 5;//fail
	*		}
	*	}
	*	2)
	*	const_cast can be used to pass const data to a function that doesn¡¯t receive const.
	*	For example, in the following program fun() receives a normal pointer,
	*	but a pointer to a const can be passed with the help of const_cast.
	*	Example:
	*	--------
	*	int fun(int* ptr) { return (*ptr + 10); }
	*	const int val = 10;
	*	const int *ptr = &val;
	*	int *ptr1 = const_cast <int *>(ptr);
	*	cout << fun(ptr1); //success
	*/
	template<class Other>
	AutoPtr<Other> unsafeCast() const
	{
		Other* pOther = const_cast<Other*>(_ptr);
		return AutoPtr(pOther, true);
	}
#endif

public:
	/**
	*	override operator =
	*/
	AutoPtr& operator = (C* ptr)
	{
		return assign(ptr);
	}

	AutoPtr& operator = (C& ptr)
	{
		return assign(ptr);
	}

	AutoPtr& operator = (const AutoPtr& ptr) const
	{
		return assign(ptr);
	}

#if defined(_MSC_VER)
#if _MSC_VER>=1300
	template<class Other>
	AutoPtr& operator = (const AutoPtr<Other>& ptr)
	{
		return assign<Other>(ptr);
	}
#endif
#else
	template<class Other>
	AutoPtr& operator = (const AutoPtr<Other>& ptr)
	{
		return assign<Other>(ptr);
	}
#endif

	/**
	*	override operator * and ->
	*/
	C* operator ->()
	{
		return _ptr;
	}

	const C* operator ->()const
	{
		return _ptr;
	}

	C& operator*()
	{
		return *_ptr;
	}

	const C& operator*()const
	{
		return *_ptr;
	}

	/**
	*	override operator ! and const C* and C*
	*/
	bool operator !() const
	{
		return _ptr == 0;
	}

	operator const C* ()
	{
		return _ptr;
	}

	operator C* ()
	{
		return _ptr;
	}

	/**
	*	override operator ==
	*/
	bool operator == (const AutoPtr& ptr) const
	{
		return _ptr == ptr._ptr;
	}

	bool operator == (const C* ptr) const
	{
		return _ptr == ptr;
	}

	bool operator == (C* ptr) const
	{
		return _ptr == ptr;
	}

	/**
	*	override operator !=
	*/
	bool operator != (const AutoPtr& ptr) const
	{
		return _ptr != ptr._ptr;
	}

	bool operator != (const C* ptr) const
	{
		return _ptr != ptr;
	}

	bool operator != (C* ptr) const
	{
		return _ptr != ptr;
	}

	/**
	*	override operator <
	*/
	bool operator < (const AutoPtr& ptr) const
	{
		return _ptr < ptr._ptr;
	}

	bool operator < (const C* ptr) const
	{
		return _ptr < ptr;
	}

	bool operator < (C* ptr) const
	{
		return _ptr < ptr;
	}

	/**
	*	override operator >
	*/
	bool operator > (const AutoPtr& ptr) const
	{
		return _ptr > ptr._ptr;
	}

	bool operator > (const C* ptr) const
	{
		return _ptr > ptr;
	}

	bool operator > (C* ptr) const
	{
		return _ptr > ptr;
	}

	/**
	*	override operator >=
	*/
	bool operator >= (const AutoPtr& ptr) const
	{
		return _ptr >= ptr._ptr;
	}

	bool operator >= (const C* ptr) const
	{
		return _ptr >= ptr;
	}

	bool operator >= (C* ptr) const
	{
		return _ptr >= ptr;
	}
};

#endif
