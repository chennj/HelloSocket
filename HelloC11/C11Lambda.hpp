#pragma once
/*	lambda表达式 拉曼大表达式 匿名函数
	[ caputrue		] ( params	) opt		-> ret			{ body;		};
	[ 外部变量捕获列表	] ( 参数列表	) 特殊操作符	-> 返回值类型		{ 函数体;	};
	---------------------------------------------------------------------
	A.外部变量捕获列表：lambda表达式的捕获列表精细控制了lambda表达式能够访问的外部变量，以及如何访问这些变量。
	1) []		: 不捕获任何变量
	2) [&]		: 捕获外部作用域中的所有变量，并作为引用在函数体中使用（按引用捕获）。
	3) [=]		: 捕获外部作用域中的所有变量，并作为副本在函数体中使用（按值捕获）。
	4) [a]		: 按值类型捕获a变量
	5) [&a]		: 按引用类型捕获a变量
	6) [&,a]	: 值类型捕获a，引用类型捕获其余变量
	7) [=,&a]	: 引用类型捕获a，其余按值类型捕获
	8) [this]	: 可以使用 Lambda 所在类中的成员变量。
	当然参数也可以是多个,比如[a,b,c,&d] [&a,b,&c,d]这样都是可以的。
	B.特殊操作符: 可以使用mutable,exception,attribute(选填)
	mutable		: 表明lambda表达式体内的代码可以修改被捕获的变量，并且可以访问被捕获的对象的non-const方法
	exception	: 表明lambda表达式是否抛出异常以及何种异常
	attribute	: 用来申明属性
*/
#include <functional>

// 普通静态函数指针
typedef void(*func_def_pointer_1)();
typedef int(*func_def_pointer_2)(int);
// 成员函数指针
class TestClass{};
typedef int(TestClass::*FP_ADD2)(int a, int b);

void fun_a()
{
	printf("func_a\n");
}

void fun_b(func_def_pointer_1 f = nullptr, std::function<void()> f1=nullptr)
{
	if (f)f();
	if (f1)f1();

	printf("func_b\n");
}

// -------------------------------
int fun_c(int n)
{

}
void test_1()
{
	// 老式的函数指针
	// 不友好
	printf("---- demo about funciton pointer which is c style and funciton pointer which is c++11 style----\n");
	printf("\nold fashion function pointer\n");
	void (*func_pointer)() = fun_a;
	func_pointer();
	fun_b(func_pointer,nullptr);

	// 新式的函数指针
	// 友好
	printf("\nnew fashion function pointer\n");
	std::function<void()> call = fun_a;
	call();
	fun_b(nullptr,call);

	printf("\n");
}

void test_2()
{
	printf("---- demo about lambda expression----\n");
	printf("\nnot parameter list and not return value\n");
	std::function<void()> call_1;
	int n = 5;
	int a = 8;
	call_1 = [/*外部变量捕获列表*/](/*参数列表*/)
	{
		printf("this lambda,没有捕获\n");
	};
	call_1();

	call_1 = [&n/*外部变量捕获列表*/](/*参数列表*/)
	{
		printf("this lambda,捕获n=%d\n",n);
	};
	call_1();

	n = 6;
	call_1 = [n/*外部变量捕获列表*/](/*参数列表*/)
	{
		printf("this lambda,捕获n=%d\n", n);
	};
	call_1();

	n = 7;
	int b = 9;
	call_1 = [&/*外部变量捕获列表*/](/*参数列表*/)
	{
		printf("this lambda,捕获所有=%d,%d,%d\n", a,b,n);
	};
	call_1();

	printf("\nnot parameter list and have return value\n");
	std::function<int()> call_2;
	call_2 = [&]()->int/*返回值*/
	{
		return a + 10;
	};
	printf("this lambda,%d\n", call_2());

	printf("\nhave parameter list and have return value\n");
	std::function<int(int)> call_3;
	call_3 = [](int la)->int/*返回值*/
	{
		return la + 10;
	};
	printf("this lambda,%d\n", call_3(5));

	call_3 = [&](int a)->int/*返回值*/
	{
		return a + b;
	};
	printf("this lambda,%d\n", call_3(5));
}

/*

int i = 42;
auto lambda = [&i](int j) { return i + j; };
std::cout << lambda(10);
The compiler basically generates this code for you:

class Lambda {
public:
Lambda(const int& i) : i(i) { }
auto operator()(int j) { return i + j; }
private:
const int& i;
};

int i = 42;
auto lambda = Lambda(i);
std::cout << lambda(10);
*/