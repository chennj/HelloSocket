#pragma once
/*	lambda���ʽ ��������ʽ ��������
	[ caputrue		] ( params	) opt		-> ret			{ body;		};
	[ �ⲿ���������б�	] ( �����б�	) ���������	-> ����ֵ����		{ ������;	};
	---------------------------------------------------------------------
	A.�ⲿ���������б�lambda���ʽ�Ĳ����б�ϸ������lambda���ʽ�ܹ����ʵ��ⲿ�������Լ���η�����Щ������
	1) []		: �������κα���
	2) [&]		: �����ⲿ�������е����б���������Ϊ�����ں�������ʹ�ã������ò��񣩡�
	3) [=]		: �����ⲿ�������е����б���������Ϊ�����ں�������ʹ�ã���ֵ���񣩡�
	4) [a]		: ��ֵ���Ͳ���a����
	5) [&a]		: ���������Ͳ���a����
	6) [&,a]	: ֵ���Ͳ���a���������Ͳ����������
	7) [=,&a]	: �������Ͳ���a�����ఴֵ���Ͳ���
	8) [this]	: ����ʹ�� Lambda �������еĳ�Ա������
	��Ȼ����Ҳ�����Ƕ��,����[a,b,c,&d] [&a,b,&c,d]�������ǿ��Եġ�
	B.���������: ����ʹ��mutable,exception,attribute(ѡ��)
	mutable		: ����lambda���ʽ���ڵĴ�������޸ı�����ı��������ҿ��Է��ʱ�����Ķ����non-const����
	exception	: ����lambda���ʽ�Ƿ��׳��쳣�Լ������쳣
	attribute	: ������������
*/
#include <functional>

// ��ͨ��̬����ָ��
typedef void(*func_def_pointer_1)();
typedef int(*func_def_pointer_2)(int);
// ��Ա����ָ��
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
	// ��ʽ�ĺ���ָ��
	// ���Ѻ�
	printf("---- demo about funciton pointer which is c style and funciton pointer which is c++11 style----\n");
	printf("\nold fashion function pointer\n");
	void (*func_pointer)() = fun_a;
	func_pointer();
	fun_b(func_pointer,nullptr);

	// ��ʽ�ĺ���ָ��
	// �Ѻ�
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
	call_1 = [/*�ⲿ���������б�*/](/*�����б�*/)
	{
		printf("this lambda,û�в���\n");
	};
	call_1();

	call_1 = [&n/*�ⲿ���������б�*/](/*�����б�*/)
	{
		printf("this lambda,����n=%d\n",n);
	};
	call_1();

	n = 6;
	call_1 = [n/*�ⲿ���������б�*/](/*�����б�*/)
	{
		printf("this lambda,����n=%d\n", n);
	};
	call_1();

	n = 7;
	int b = 9;
	call_1 = [&/*�ⲿ���������б�*/](/*�����б�*/)
	{
		printf("this lambda,��������=%d,%d,%d\n", a,b,n);
	};
	call_1();

	printf("\nnot parameter list and have return value\n");
	std::function<int()> call_2;
	call_2 = [&]()->int/*����ֵ*/
	{
		return a + 10;
	};
	printf("this lambda,%d\n", call_2());

	printf("\nhave parameter list and have return value\n");
	std::function<int(int)> call_3;
	call_3 = [](int la)->int/*����ֵ*/
	{
		return la + 10;
	};
	printf("this lambda,%d\n", call_3(5));

	call_3 = [&](int a)->int/*����ֵ*/
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