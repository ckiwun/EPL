#ifndef _Vector_H_
#define _Vector_H_

#include <cstdint>
#include <stdexcept>
#include <utility>

//Utility gives std::rel_ops which will fill in relational
//iterator operations so long as you provide the
//operators discussed in class.  In any case, ensure that
//all operations listed in this website are legal for your
//iterators:
//http://www.cplusplus.com/reference/iterator/RandomAccessIterator/
using namespace std::rel_ops;
using std::cout;
using std::endl;
using std::cout;
using std::endl;

namespace epl{

class invalid_iterator {
public:
	enum SeverityLevel { SEVERE, MODERATE, MILD, WARNING };
	SeverityLevel level;

	invalid_iterator(SeverityLevel level = SEVERE) { this->level = level; }
	virtual const char* what() const {
		switch (level) {
		case WARNING:   return "Warning"; // not used in Spring 2015
		case MILD:      return "Mild";
		case MODERATE:  return "Moderate";
		case SEVERE:    return "Severe";
		default:        return "ERROR"; // should not be used
		}
	}
};

template <typename T>
struct allocator {
	T* allocate(uint64_t num) const {
		return (T*) ::operator new(num * sizeof(T));
	}
	void deallocate(T* p) const {
		::operator delete(p);
	}
	void construct(T* p) const {
		new(p) T{};
	}
	void destroy(T* p) const { p->~T(); }
};

template <typename T>
class vector {
private:
	void copy(vector<T> const& that) {
		//cout << "copy\n";
		//cout << that.head_ptr << endl;
		//cout << that.tail_ptr << endl;
		//cout << that.f_zero << endl;
		//cout << that.capacity << endl;
		head_ptr = that.head_ptr;
		tail_ptr = that.tail_ptr;
		f_zero = that.f_zero;
		capacity = that.capacity;
		version = 100;
		realloc_version = 100;
		if (size() == 0) {
			object = nullptr;
			return;
		}
		object = my_alloc.allocate(capacity); //allocate memory
		for (uint32_t k = 0; k < size(); k++)
			new(object + index(k)) T(that.object[index(k)]);
	}
	void destroy() {
		//cout << "destroy\n";
		for (uint32_t k = 0; k < size(); k++) {
			object[index(k)].~T(); //call destructor 
		}
		my_alloc.deallocate(object);
	}
	void move(vector<T>&& tmp) {
		head_ptr = tmp.head_ptr;
		tail_ptr = tmp.tail_ptr;
		f_zero = tmp.f_zero;
		capacity = tmp.capacity;
		object = tmp.object;
		version = 100;
		realloc_version = 100;
		tmp.head_ptr = 0;
		tmp.tail_ptr = 0;
		tmp.f_zero = 1;
		tmp.capacity = default_capacity;
		tmp.object = nullptr;
		tmp.version = version;
		tmp.realloc_version = realloc_version;
	}
	void resize() {
		//amortized doubling
		/*cout << "resize\n";
		cout << "orig tail" << tail_ptr << endl;
		cout << "orig head" << head_ptr << endl;
		cout << "orig capacity" << capacity << endl;*/
		T* new_object = my_alloc.allocate(capacity * 2);
		for (uint32_t k = 0; k <= tail_ptr; k++) {
			new(new_object + k) T(std::move(object[k]));
			object[k].~T();
		}
		if (head_ptr != 0) {//special case, all should have been moved to by "tail"
			for (uint32_t k = head_ptr; k < capacity; k++) {
				new(new_object + capacity + k) T(std::move(object[k]));
				object[k].~T();
			}
			head_ptr += capacity;
		}
		capacity *= 2;
		/*cout << "new tail" << tail_ptr << endl;
		cout << "new head" << head_ptr << endl;
		cout << "new capacity" << capacity << endl;
		cout << "orig object points to " << object << endl;
		cout << "new object points to " << new_object << endl;*/
		my_alloc.deallocate(object);
		object = new_object;
		realloc_version++;
	}
	int index(uint64_t n) { //find n-th index
		int res = head_ptr;
		uint64_t count = n;
		while (count > 0) {
			res++;
			round(res);
			count--;
		}
		return res;
	}
	void round(int& ptr) {
		if (ptr<0)	ptr += capacity;
		if (ptr >= capacity) ptr -= capacity;
	}

private:
	T* object;
	int head_ptr;
	int tail_ptr;
	int f_zero;
	uint64_t capacity;
	allocator<T> my_alloc{};
	uint64_t version;
	uint64_t realloc_version;

	static constexpr uint64_t default_capacity = 8;
// Project 1C
private:
	template <typename Elem> class iterator_helper
		: public std::iterator<std::random_access_iterator_tag, Elem> {
	private:
		Elem* ptr;
		vector* home;
		uint64_t version;
		uint64_t realloc_version;
		using Same = iterator_helper<Elem>;
	public:
		Elem& operator*(void) {
			//check exception
			bool out_of_bound = true;
			for (int i = 0; i < home->size(); i++) if (ptr == home->object+(home->index(i)) ) out_of_bound = false;
			if (ptr == home->object + (home->size())) out_of_bound = false;
			if((home->realloc_version == this->realloc_version)&& (home->version == this->version) && out_of_bound) throw ::epl::invalid_iterator(::epl::invalid_iterator::SEVERE);
			else if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
			else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
			return ptr[0];
		}
		Same& operator++(void) {//prefix, i.e., ++x
			++ptr;
			if (ptr > (home->object + home->capacity)) ptr -= home->capacity;
			if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
			else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
			return *this;
		}
		Same operator++(int) {//postfix, i.e., x++
			Same t{ *this }; // make a copy
			operator++(); // increment myself
			return t;
		}
	
		bool operator==(Same const& rhs) const {
			if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
			else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
			if (rhs.home->realloc_version != rhs.realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
			else if (rhs.home->version != rhs.version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
			return this->ptr == rhs.ptr;
		}
	
		Same& operator--(void) {//prefix, i.e., --x
			--ptr;
			if (ptr < home->object) ptr += home->capacity;
			if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
			else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
			return *this;
		}
		Same operator--(int) {//postfix, i.e., x--
			Same t{ *this }; // make a copy
			operator--(); // increment myself
			return t;
		}
	
		Same operator+(int32_t k) {
			Same result{};
			result.ptr = this->ptr + k;
			if (result.ptr > (home->object + home->capacity)) ptr -= home->capacity;
			if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
			else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
		}
	
		Same operator-(int32_t k) {
			Same result{};
			result.ptr = this->ptr - k;
			if (result.ptr < home->object) ptr += home->capacity;
			if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
			else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
		}
	
		iterator_helper(vector const * rhs,uint64_t ver,uint64_t realloc_ver) :version(ver),realloc_version(realloc_ver) { home = const_cast<vector*>(rhs); }
		iterator_helper(vector * rhs, uint64_t ver, uint64_t realloc_ver) : home(rhs), version(ver), realloc_version(realloc_ver) {}
		iterator_helper(uint64_t ver, uint64_t realloc_ver) :version(ver), realloc_version(realloc_ver) {	home = nullptr;	}
		friend vector;
	};

	//fail: iterator to const_iterator
	//class const_iterator : public std::iterator<std::random_access_iterator_tag, T> {
	//private:
	//	using Elem = T;
	//	Elem* ptr;
	//	vector* home;
	//	uint64_t version;
	//	uint64_t realloc_version;
	//	using Same = const_iterator;
	//public:
	//	Elem& operator*(void) {
	//		//check exception
	//		bool out_of_bound = true;
	//		for (int i = 0; i < home->size(); i++) if (ptr == home->object+(home->index(i)) ) out_of_bound = false;
	//		if (ptr == home->object + (home->size())) out_of_bound = false;
	//		if((home->realloc_version == this->realloc_version)&& (home->version == this->version) && out_of_bound) throw ::epl::invalid_iterator(::epl::invalid_iterator::SEVERE);
	//		else if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
	//		else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
	//		return ptr[0];
	//	}
	//	Same& operator++(void) {//prefix, i.e., ++x
	//		++ptr;
	//		if (ptr > (home->object + home->capacity)) ptr -= home->capacity;
	//		if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
	//		else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
	//		return *this;
	//	}
	//	Same operator++(int) {//postfix, i.e., x++
	//		Same t{ *this }; // make a copy
	//		operator++(); // increment myself
	//		return t;
	//	}
	//
	//	bool operator==(Same const& rhs) const {
	//		if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
	//		else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
	//		if (rhs.home->realloc_version != rhs.realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
	//		else if (rhs.home->version != rhs.version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
	//		return this->ptr == rhs.ptr;
	//	}
	//
	//	Same& operator--(void) {//prefix, i.e., --x
	//		--ptr;
	//		if (ptr < home->object) ptr += home->capacity;
	//		if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
	//		else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
	//		return *this;
	//	}
	//	Same operator--(int) {//postfix, i.e., x--
	//		Same t{ *this }; // make a copy
	//		operator--(); // increment myself
	//		return t;
	//	}
	//
	//	Same operator+(int32_t k) {
	//		Same result{};
	//		result.ptr = this->ptr + k;
	//		if (result.ptr > (home->object + home->capacity)) ptr -= home->capacity;
	//		if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
	//		else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
	//	}
	//
	//	Same operator-(int32_t k) {
	//		Same result{};
	//		result.ptr = this->ptr - k;
	//		if (result.ptr < home->object) ptr += home->capacity;
	//		if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
	//		else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
	//	}
	//
	//	const_iterator(vector const * rhs,uint64_t ver,uint64_t realloc_ver) :version(ver),realloc_version(realloc_ver) { home = const_cast<vector*>(rhs); }
	//	const_iterator(vector * rhs, uint64_t ver, uint64_t realloc_ver) : home(rhs), version(ver), realloc_version(realloc_ver) {}
	//	const_iterator(uint64_t ver, uint64_t realloc_ver) :version(ver), realloc_version(realloc_ver) { home = nullptr;	}
	//	const_iterator(void) : home(nullptr), version(0), realloc_version(0) {}
	//	friend vector;
	//};

	//class iterator : public const_iterator {
	//private:
	//	using Elem = T;
	//	Elem* ptr;
	//	vector* home;
	//	uint64_t version;
	//	uint64_t realloc_version;
	//	using Same = const_iterator;
	//public:
	//	Elem& operator*(void) {
	//		//check exception
	//		bool out_of_bound = true;
	//		for (int i = 0; i < home->size(); i++) if (ptr == home->object + (home->index(i))) out_of_bound = false;
	//		if (ptr == home->object + (home->size())) out_of_bound = false;
	//		if ((home->realloc_version == this->realloc_version) && (home->version == this->version) && out_of_bound) throw ::epl::invalid_iterator(::epl::invalid_iterator::SEVERE);
	//		else if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
	//		else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
	//		return ptr[0];
	//	}
	//	Same& operator++(void) {//prefix, i.e., ++x
	//		++ptr;
	//		if (ptr >(home->object + home->capacity)) ptr -= home->capacity;
	//		if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
	//		else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
	//		return *this;
	//	}
	//	Same operator++(int) {//postfix, i.e., x++
	//		Same t{ *this }; // make a copy
	//		operator++(); // increment myself
	//		return t;
	//	}

	//	bool operator==(Same const& rhs) const {
	//		if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
	//		else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
	//		if (rhs.home->realloc_version != rhs.realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
	//		else if (rhs.home->version != rhs.version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
	//		return this->ptr == rhs.ptr;
	//	}

	//	Same& operator--(void) {//prefix, i.e., --x
	//		--ptr;
	//		if (ptr < home->object) ptr += home->capacity;
	//		if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
	//		else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
	//		return *this;
	//	}
	//	Same operator--(int) {//postfix, i.e., x--
	//		Same t{ *this }; // make a copy
	//		operator--(); // increment myself
	//		return t;
	//	}

	//	Same operator+(int32_t k) {
	//		Same result{};
	//		result.ptr = this->ptr + k;
	//		if (result.ptr > (home->object + home->capacity)) ptr -= home->capacity;
	//		if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
	//		else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
	//	}

	//	Same operator-(int32_t k) {
	//		Same result{};
	//		result.ptr = this->ptr - k;
	//		if (result.ptr < home->object) ptr += home->capacity;
	//		if (home->realloc_version != this->realloc_version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MODERATE);
	//		else if (home->version != this->version) throw ::epl::invalid_iterator(::epl::invalid_iterator::MILD);
	//	}

	//	iterator(vector const * rhs, uint64_t ver, uint64_t realloc_ver) :version(ver), realloc_version(realloc_ver) { home = const_cast<vector*>(rhs); }
	//	iterator(vector * rhs, uint64_t ver, uint64_t realloc_ver) : home(rhs), version(ver), realloc_version(realloc_ver) {}
	//	iterator(uint64_t ver, uint64_t realloc_ver) :version(ver), realloc_version(realloc_ver) { home = nullptr; }
	//	friend vector;
	//};
public:
	using iterator = iterator_helper<T>;
	using const_iterator = iterator_helper<const T>;
	
	iterator begin(void) {
		iterator p(this,version,realloc_version);
		p.ptr = object + head_ptr;
		return p;
	}
	iterator end(void) {
		iterator p(this,version, realloc_version);
		p.ptr = object + tail_ptr + 1;
		return p;
	}
	const_iterator begin(void) const{
		const_iterator p(this,version, realloc_version);
		p.ptr = object + head_ptr;
		return p;
	}
	const_iterator end(void) const{
		const_iterator p(this,version, realloc_version);
		p.ptr = object + tail_ptr + 1;
		return p;
	}
	
	//template constructor
	template <typename It>
		vector(It begin, It end) {
			if (typeid(typename std::iterator_traits<It>::iterator_category).name() == typeid(std::random_access_iterator_tag).name()) {
				uint32_t length = end - begin;
				head_ptr = 0;
				tail_ptr = length - 1;
				f_zero = 0;
				capacity = length;
				version = 0;
				realloc_version = 0;
				object = my_alloc.allocate(length);
				for (int k = 0; k < length; k++,begin++)
					new (object + k) T(*begin);
			}
			else {
				head_ptr = 0;
				tail_ptr = 0;
				f_zero = 1;
				capacity = default_capacity;
				object = my_alloc.allocate(default_capacity);
				version = 0;
				realloc_version = 0;
				while (begin != end) {
					this->push_back(*begin);
					++begin;
				}
			}
	}

	vector(std::initializer_list<T> list) : vector(list.begin(), list.end()) {}

	//variadic function
	template<typename... Args>
	void emplace_back(Args&&... args) {
		//cout << "emplace back\n";
		int index = -1;
		bool dup_event = false;
		bool isresize = false;
		for (int i = 0; i < size(); i++) {
			if (&object[i] == T{std::forward<Args>(args)...}) {
				index = i;
				dup_event = true;
			}
		}
		if (size() == capacity) {
			isresize = true;
			resize();
		}
		if (size() == 0)
			f_zero = 0;
		else {
			tail_ptr++;
			round(tail_ptr);
		}
		if (dup_event&&isresize)
			new(object + tail_ptr) T(object[index]);
		else
			new(object + tail_ptr) T{ std::forward<Args>(args)... };
		version++;
	}
// project 1c end here
public:
	vector(void) {
		head_ptr = 0;
		tail_ptr = 0;
		f_zero = 1;
		capacity = default_capacity;
		object = my_alloc.allocate(default_capacity);
		version = 0;
		realloc_version = 0;
	}

	explicit vector(uint64_t n) {
		if (n == 0) {
			head_ptr = 0;
			tail_ptr = 0;
			f_zero = 1;
			capacity = default_capacity;
			object = my_alloc.allocate(default_capacity);
			version = 0;
			realloc_version = 0;
			return;
		}
		head_ptr = 0;
		tail_ptr = n - 1;
		f_zero = 0;
		capacity = n;
		object = my_alloc.allocate(capacity);
		version = 0;
		realloc_version = 0;
		for (int k = 0; k < n; k++)
			new (object + k) T();
	}

	vector(vector<T> const& rhs) {
		//cout << "rhs head is " << rhs.head_ptr << endl;
		copy(rhs);
	}

	vector<T>& operator=(vector<T> const& rhs) {
		if (this != &rhs) {
			destroy();
			copy(rhs);
		}
		return *this;
	}

	vector(vector<T>&& tmp) {
		//cout << "vector move constructor\n";
		this->move(std::move(tmp));
	}

	

	vector<T>& operator=(vector<T>&& rhs) {
		//cout << "guess this not called\n";
		//if (this != &rhs)
		//{
		destroy();
		object = rhs.object;
		tail_ptr = rhs.tail_ptr;
		head_ptr = rhs.head_ptr;
		f_zero = rhs.f_zero;
		capacity = rhs.capacity;
		version = 100;
		realloc_version = 100;
		rhs.head_ptr = 0;
		rhs.tail_ptr = 0;
		rhs.f_zero = 1;
		rhs.capacity = default_capacity;
		rhs.object = nullptr;
		rhs.version = 0;
		rhs.realloc_version = 0;
		//}
		return *this;
	}


	~vector(void) {
		destroy();
	}

	uint64_t size(void) const {
		if (f_zero == 1)
			return 0;
		else {
			if (head_ptr > tail_ptr)
				return tail_ptr + capacity - head_ptr + 1;
			else if (head_ptr < tail_ptr)
				return tail_ptr - head_ptr + 1;
			else
				return 1;
		}
	}

	T& operator[](int k) {
		if (k >= size() || k<0)
			throw std::out_of_range("out of range!\n");
		return object[index(k)];
	}

	const T& operator[](int k) const {
		if (k >= size() || k<0)
			throw std::out_of_range("out of range!\n");
		return object[index(k)];
	}

	void push_back(const T& rhs) {
		//cout << "lvalue push back\n";
		int index = -1;
		bool dup_event = false;
		bool isresize = false;
		for (int i = 0; i < size(); i++) {
			if (&object[i] == &rhs) {
				index = i;
				dup_event = true;
			}
		}
		if (size() == capacity) {
			isresize = true;
			resize();
		}
		if (size() == 0)
			f_zero = 0;
		else {
			tail_ptr++;
			round(tail_ptr);
		}
		if (dup_event&&isresize)
			new(object + tail_ptr) T(object[index]);
		else
			new(object + tail_ptr) T(rhs);
		version++;
	}

	void push_back(T&& rhs) {
		//cout << "rvalue push back\n";
		if (size() == capacity)
			resize();
		if (size() == 0)
			f_zero = 0;
		else {
			tail_ptr++;
			round(tail_ptr);
		}
		new(object + tail_ptr) T(std::move(rhs));
		version++;
	}

	void push_front(const T& rhs) {
		//cout << "lvalue push front\n";
		if (size() == capacity)
			resize();
		if (size() == 0)
			f_zero = 0;
		else {
			head_ptr--;
			round(head_ptr);
		}
		new(object + head_ptr) T(rhs);
		version++;
	}

	void push_front(T&& rhs) {
		//cout << "rvalue push front\n";
		if (size() == capacity)
			resize();
		if (size() == 0)
			f_zero = 0;
		else {
			head_ptr--;
			round(head_ptr);
		}
		new(object + head_ptr) T(std::move(rhs));
		version++;
	}

	void pop_back(void) {
		if (size() == 0)
			throw std::out_of_range("out of range!\n");
		else if (size() == 1)
			f_zero = 1;
		object[tail_ptr].~T();
		tail_ptr--;
		round(tail_ptr);
		version++;
	}

	void pop_front(void) {
		if (size() == 0)
			throw std::out_of_range("out of range!\n");
		else if (size() == 1)
			f_zero = 1;
		object[head_ptr].~T();
		head_ptr++;
		round(head_ptr);
		version++;
	}
};
} //namespace epl

#endif
