// Valarray.h

/* Put your solution in this file, we expect to be able to use
 * your epl::valarray class by simply saying #include "Valarray.h"
 *
 * We will #include "Vector.h" to get the epl::vector<T> class 
 * before we #include "Valarray.h". You are encouraged to test
 * and develop your class using std::vector<T> as the base class
 * for your epl::valarray<T>
 * you are required to submit your project with epl::vector<T>
 * as the base class for your epl::valarray<T>
 */

#ifndef _Valarray_h
#define _Valarray_h
#include <vector>
#include <iostream>
#include <cstdint>
#include <complex>
#include <algorithm>
#include <functional>
#include <cmath>
#include "Vector.h"
//namespace epl{
//using std::vector; // during development and testing
using std::complex;
using epl::vector; // after submission

template <typename T>
class Smartvalarray;

template <typename>
struct SRank;

template <> struct SRank<int> { static constexpr int value = 1;  };
template <> struct SRank<float> { static constexpr int value = 2; };
template <> struct SRank<double> { static constexpr int value = 3; };
template <> struct SRank<complex<float>> { static constexpr int value = 4; };
template <> struct SRank<complex<double>> { static constexpr int value = 5; };

template <int>
struct SType;

template <> struct SType<1> { using type = int; };
template <> struct SType<2> { using type = float; };
template <> struct SType<3> { using type = double; };
template <> struct SType<4> { using type = complex<float>; };
template <> struct SType<5> { using type = complex<double>; };

//template <typename T, bool is_complex>

template <typename T1, typename T2>
struct choose_type {

	static constexpr int t1_rank = SRank<T1>::value;
	static constexpr int t2_rank = SRank<T2>::value;
	//static constexpr int max_rank = std::max(t1_rank, t2_rank);
	static constexpr int max_rank = (t1_rank==3&t2_rank==4)||(t1_rank==4&t2_rank==3)? 5 :t1_rank > t2_rank ? t1_rank : t2_rank;

	using type = typename SType<max_rank>::type;
};

template <typename T1, typename T2>
using ChooseType = typename choose_type<T1, T2>::type;

template <typename T> struct to_ref { using type = T; };
template <typename T> struct to_ref<Smartvalarray<T>> { using type = const Smartvalarray<T>&; }; //use reference if type is smartvalarray
template <typename T> using ChooseRef = typename to_ref<T>::type;

template <bool p> struct ctype;
template <> struct ctype<true> { using type = complex<double>; };
template <> struct ctype<false> { using type = double; };

template <typename T> struct choose_sqrt_type{
	static constexpr int trank = SRank<T>::value;
	static constexpr bool iscomplex = (trank==4||trank==5)?true:false;
	using type = typename ctype<iscomplex>::type;
};
template <typename T>
using ChooseSqrtType = typename choose_sqrt_type<T>::type;

template <typename T>
class my_sqrt{
public:
	using result_type = ChooseSqrtType<T>;
	result_type operator()(T const& arg) const{ 
		auto result = std::sqrt(arg);
		return static_cast<result_type>(result);
	}
};

template <typename T>
class iterator:public std::iterator<std::forward_iterator_tag, T>{
	T const _home;
	uint64_t _index;
public:
	iterator(T const& home,uint64_t index):_home(home),_index(index){}
	iterator(iterator const& rhs) = default;
	using Same = iterator<T>;
	typename T::value_type operator*() const{return _home[_index];} //where real evalation happen
	Same& operator++(){//prefix, ++a
		_index++;
		return *this;
	}
	Same operator++(int){//postfix, a++
		Same tmp(*this);
		this->operator++();
		return tmp;
	}
	Same& operator--(){//prefix, --a
		_index--;
		return *this;
	}
	Same operator--(int){//postfix, a--
		Same tmp(*this);
		this->operator--();
		return tmp;
	}
	bool operator==(Same const& rhs){
		return this->_home==rhs._home&&this->_index==rhs._index;
	}
	
	bool operator!=(Same const& rhs){
		return !(this->operator==(rhs));
	}
};

template <typename LEFT,typename RIGHT,typename FUN>
class Proxy{
	ChooseRef<LEFT> left;
	ChooseRef<RIGHT> right;
	FUN op;
public:
	Proxy(ChooseRef<LEFT> left_t,ChooseRef<RIGHT> right_t,FUN op_t):left(left_t),right(right_t),op(op_t){}//element to element constructor
	using value_type = ChooseType<typename LEFT::value_type, typename RIGHT::value_type>;
	uint64_t size(void) const{
		return left.size()<right.size()? left.size(): right.size();
	}
	typename FUN::result_type operator[](uint64_t index) const{
		return op(left[index],right[index]); //where real evalation happen
	}
	//phase b
	iterator<Proxy<LEFT,RIGHT,FUN>> begin(void){return iterator<Proxy<LEFT,RIGHT,FUN>>(*this,0);}
	iterator<Proxy<LEFT,RIGHT,FUN>> end(void){return iterator<Proxy<LEFT,RIGHT,FUN>>(*this,size());}
};
//unary proxy, for negate
template <typename V,typename FUN>
class UnaryProxy{
	ChooseRef<V> vec;
	FUN op;
public:
	UnaryProxy(ChooseRef<V> vec_t,FUN op_t):vec(vec_t),op(op_t){}//element to element constructor
    using value_type = typename V::value_type;
	uint64_t size(void) const{
		return vec.size();
	}
	typename FUN::result_type operator[](uint64_t index) const{
		return op(vec[index]); //where real evalation happen
	}
	//phase b
	iterator<UnaryProxy<V,FUN>> begin(void){return iterator<UnaryProxy<V,FUN>>(*this,0);}
	iterator<UnaryProxy<V,FUN>> end(void){return iterator<UnaryProxy<V,FUN>>(*this,size());}
};

template <typename T>
class Scalar{
	T value;
public:
	Scalar(T v):value(v){}
    using value_type = T;
	uint64_t size(void) const{
		return UINT64_MAX;
	}
	T operator[](uint64_t index) const{
		return value;
	}
	//phase b
	iterator<Scalar<T>> begin(void){return iterator<Scalar<T>>(*this,0);}
	iterator<Scalar<T>> end(void){return iterator<Scalar<T>>(*this,size());}
};

template <typename S>
struct Wrap : public S{
	using S::S;
	using Stype = typename S::value_type;
	Wrap(S const& s) : S(s) {}
	Wrap(void) : S() {}
	explicit Wrap(uint64_t n) : S(n) {}
	template <typename T>
	Wrap(std::initializer_list<T> list) : S(list) {}
	template <typename T>
	Wrap(Wrap<T> const& rhs) : Wrap(rhs.size()) {
		for(uint64_t i=0;i<rhs.size();i++){
			(*this)[i] = static_cast<Stype>(rhs[i]);
		}
	}
	Wrap& operator=(Wrap<S> const& rhs){
		uint64_t size = std::min(this->size(), rhs.size());
		for (uint64_t k = 0; k < size; k++) {
			(*this)[k] = static_cast<Stype>(rhs[k]);
		}
		return *this;
	}
	
	template <typename T>
	Wrap& operator=(Wrap<T> const& rhs){
		uint64_t size = std::min(this->size(), rhs.size());
		for (uint64_t k = 0; k < size; k++) {
			(*this)[k] = static_cast<Stype>(rhs[k]);
		}
		return *this;
	}

	template <typename FUN>
	typename FUN::result_type accumulate(FUN op) {
		S const& real_this(*this);
		if(real_this.size()==0) return 0;
		typename FUN::result_type result(real_this[0]);
		for (int i=1;i<real_this.size();i++)
			result = op(result, real_this[i]);
		return result;
	}
	
	typename std::plus<Stype>::result_type sum(void) {
		return accumulate(std::plus<Stype>{});
	}
	
	template <typename FUN>
	Wrap<UnaryProxy<S, FUN>> apply(FUN op) {
		S const& real_this(*this);
		return Wrap<UnaryProxy<S,FUN>>(UnaryProxy<S,FUN>(real_this, op));
	}
	
	Wrap<UnaryProxy<S,std::negate<Stype>>> operator-(void) {
	    return apply(std::negate<Stype>{});
	}
	
	Wrap<UnaryProxy<S,my_sqrt<Stype>>> sqrt(void) {
	    return apply(my_sqrt<Stype>{});
	}
};

template <typename T>
class Smartvalarray : public vector<T> {
	using Same = Smartvalarray<T>;
public:
	using vector<T>::vector;
	//These methods are inherited from vector:
	//size()
	//operator[]
	//value_type
	
	//since lhs should always be Wrap<Smartvalarray> so put operator= here, Wrap<others> should not call operator=
	//Same& operator=(Same const& rhs) {
	//	Same& lhs = *this;
	//	uint64_t size = std::min(lhs.size(), rhs.size());
	//	for (uint64_t k = 0; k < size; k++) {
	//		lhs[k] = rhs[k];
	//	}
	//	return *this;
	//}
	
};

template <typename T>
using valarray = Wrap<Smartvalarray<T>>;

template <typename T1, typename T2> struct get_type;
template <typename V1, typename V2>//wrap + wrap
struct get_type<Wrap<V1>, Wrap<V2>> { using type = ChooseType<typename V1::value_type, typename V2::value_type>; };
template <typename V, typename T>//scalar + wrap
struct get_type<Wrap<V>, T> { using type = ChooseType<typename V::value_type, T>; };
template <typename T, typename V>//wrap + scalar
struct get_type<T, Wrap<V>> { using type = ChooseType<T, typename V::value_type>; };

template <bool p, typename T>
using EnableIf = typename std::enable_if<p, T>::type;

//operator+
//wrap + wrap
template <typename T1,typename T2,typename FUN = std::plus<typename get_type<Wrap<T1>,Wrap<T2>>::type> >
Wrap<Proxy<T1,T2,FUN>> operator+(Wrap<T1> const& lhs, Wrap<T2> const& rhs) {
	T1 const& real_lhs(lhs);
	T2 const& real_rhs(rhs);
	return Wrap<Proxy<T1,T2,FUN>>(Proxy<T1,T2,FUN>(real_lhs,real_rhs,FUN{}));
}
//scalar + wrap
template <typename T1,typename T2,typename FUN = std::plus<typename get_type<T1,Wrap<T2>>::type> >
EnableIf<SRank<T1>::value!=0, Wrap<Proxy<Scalar<T1>, T2, FUN>>> operator+(T1 const& lhs, Wrap<T2> const& rhs) {
	T2 const& real_rhs(rhs);
	return Wrap<Proxy<Scalar<T1>,T2,FUN>>(Proxy<Scalar<T1>,T2,FUN>(Scalar<T1>(lhs),real_rhs,FUN{}));
}
//wrap + scalar
template <typename T1,typename T2,typename FUN = std::plus<typename get_type<Wrap<T1>,T2>::type> >
EnableIf<SRank<T2>::value!=0, Wrap<Proxy<T1, Scalar<T2>, FUN>>> operator+(Wrap<T1> const& lhs, T2 const& rhs) {
	T1 const& real_lhs(lhs);
	return Wrap<Proxy<T1,Scalar<T2>,FUN>>(Proxy<T1,Scalar<T2>,FUN>(real_lhs,Scalar<T2>(rhs),FUN{}));
}
//operator-
//wrap + wrap
template <typename T1,typename T2,typename FUN = std::minus<typename get_type<Wrap<T1>,Wrap<T2>>::type> >
Wrap<Proxy<T1,T2,FUN>> operator-(Wrap<T1> const& lhs, Wrap<T2> const& rhs) {
	return Wrap<Proxy<T1,T2,FUN>>(Proxy<T1,T2,FUN>(lhs,rhs,FUN{}));
}
//scalar + wrap
template <typename T1,typename T2,typename FUN = std::minus<typename get_type<T1,Wrap<T2>>::type> >
EnableIf<SRank<T1>::value!=0, Wrap<Proxy<Scalar<T1>, T2, FUN>>> operator-(T1 const& lhs, Wrap<T2> const& rhs) {
	T2 const& real_rhs(rhs);
	return Wrap<Proxy<Scalar<T1>,T2,FUN>>(Proxy<Scalar<T1>,T2,FUN>(Scalar<T1>(lhs),real_rhs,FUN{}));
}
//wrap + scalar
template <typename T1,typename T2,typename FUN = std::minus<typename get_type<Wrap<T1>,T2>::type> >
EnableIf<SRank<T2>::value!=0, Wrap<Proxy<T1, Scalar<T2>, FUN>>> operator-(Wrap<T1> const& lhs, T2 const& rhs) {
	T1 const& real_lhs(lhs);
	return Wrap<Proxy<T1,Scalar<T2>,FUN>>(Proxy<T1,Scalar<T2>,FUN>(real_lhs,Scalar<T2>(rhs),FUN{}));
}
//operator*
//wrap + wrap
template <typename T1,typename T2,typename FUN = std::multiplies<typename get_type<Wrap<T1>,Wrap<T2>>::type> >
Wrap<Proxy<T1,T2,FUN>> operator*(Wrap<T1> const& lhs, Wrap<T2> const& rhs) {
	return Wrap<Proxy<T1,T2,FUN>>(Proxy<T1,T2,FUN>(lhs,rhs,FUN{}));
}
//scalar + wrap
template <typename T1,typename T2,typename FUN = std::multiplies<typename get_type<T1,Wrap<T2>>::type> >
EnableIf<SRank<T1>::value!=0, Wrap<Proxy<Scalar<T1>, T2, FUN>>> operator*(T1 const& lhs, Wrap<T2> const& rhs) {
	T2 const& real_rhs(rhs);
	return Wrap<Proxy<Scalar<T1>,T2,FUN>>(Proxy<Scalar<T1>,T2,FUN>(Scalar<T1>(lhs),real_rhs,FUN{}));
}
//wrap + scalar
template <typename T1,typename T2,typename FUN = std::multiplies<typename get_type<Wrap<T1>,T2>::type> >
EnableIf<SRank<T2>::value!=0, Wrap<Proxy<T1, Scalar<T2>, FUN>>> operator*(Wrap<T1> const& lhs, T2 const& rhs) {
	T1 const& real_lhs(lhs);
	return Wrap<Proxy<T1,Scalar<T2>,FUN>>(Proxy<T1,Scalar<T2>,FUN>(real_lhs,Scalar<T2>(rhs),FUN{}));
}
//operator/
//wrap + wrap
template <typename T1,typename T2,typename FUN = std::divides<typename get_type<Wrap<T1>,Wrap<T2>>::type> >
Wrap<Proxy<T1,T2,FUN>> operator/(Wrap<T1> const& lhs, Wrap<T2> const& rhs) {
	return Wrap<Proxy<T1,T2,FUN>>(Proxy<T1,T2,FUN>(lhs,rhs,FUN{}));
}
//scalar + wrap
template <typename T1,typename T2,typename FUN = std::divides<typename get_type<T1,Wrap<T2>>::type> >
EnableIf<SRank<T1>::value!=0, Wrap<Proxy<Scalar<T1>, T2, FUN>>> operator/(T1 const& lhs, Wrap<T2> const& rhs) {
	T2 const& real_rhs(rhs);
	return Wrap<Proxy<Scalar<T1>,T2,FUN>>(Proxy<Scalar<T1>,T2,FUN>(Scalar<T1>(lhs),real_rhs,FUN{}));
}
//wrap + scalar
template <typename T1,typename T2,typename FUN = std::divides<typename get_type<Wrap<T1>,T2>::type> >
EnableIf<SRank<T2>::value!=0, Wrap<Proxy<T1, Scalar<T2>, FUN>>> operator/(Wrap<T1> const& lhs, T2 const& rhs) {
	T1 const& real_lhs(lhs);
	return Wrap<Proxy<T1,Scalar<T2>,FUN>>(Proxy<T1,Scalar<T2>,FUN>(real_lhs,Scalar<T2>(rhs),FUN{}));
}

template <typename T>
std::ostream& operator<<(std::ostream& out, const Wrap<T>&  vec) {
	const char* pref = "";
	T const& real_vec{vec};
    out << "[";
    for (uint64_t i = 0; i < real_vec.size(); ++i) {
        out << real_vec[i] << ", ";
    }
    out << "]" << std::endl;
	return out;
}

//};

#endif /* _Valarray_h */

