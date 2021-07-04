/*  ˵�� ������ʵ����һ����Դ����������Ϊ ���̺߳Ͷ��̰߳汾
���̣߳������������ʵ���� ֧�ֶ��̵߳� ��Դ������
����: ʵ����һ�� ��Դ�ķ�������������Դ�Ŀ���������ͷ�
��Դ��������Զ��߳�ִ��

���̣߳� һ�����ε� ��Դ�� ˳������Դ �� ѭ������ʹ��
�����е���Դ���� ѭ��ʹ�õ����� ��  ���Բ�������ʾ���ǻ��ղ��� ,
���� T���ݽṹ�� ʹ�� ���������ʽ�������� �� ���� Ԫ���� �ṹ�в���Ҫ����ṹ�塣

// update 190313 �� ������ ˳����Դ�� ���� ��ʼ���� next �� prev ָ�� �� �����ʱ��ֻ���� ʵ�ʵ�����D ,  �ڻ�ȡ��Դʱ Ҳֻ�洢��Դ�� ������������


// ��ǰʹ�÷�Χ  ��  tick ���ݴ洢 


*/

#pragma once 
#include<iostream>  
#include<Windows.h>  

using namespace std;


//  support multly thread
#define SUP_MULT_THREAD  FALSE   //  tick ֧�ֶ��߳� 


#undef  RESOURCE_SZIE2
#define RESOURCE_SZIE2 30    // ��sing��������




template < typename T, typename T2 >

class source_manag2
{
	struct fake_data   //  fake����data �Ľṹ�� T�Ľṹ��ͷ�� �� ����ṹһ�£��ֽڶ���  
	{
		void *prev;
		T2	  D;
		void *next;
	};


private:

	volatile long count;
	volatile long volume;
	volatile T**  next_ptr;  //  ���� ����������ݵ� ��ַ  
	int lens;

	T	 data[RESOURCE_SZIE2];
	int size;



public:

	source_manag2(){};
	~source_manag2(){};

	int init_resourcetab();
	int set_next_ptr(T**  ptr);
	volatile T** get_next_ptr();


	typename T * req_resource(T2* addr, int new_volume);  // ������Դ ���洢����

	typename T * req_resource(int new_volume);  // ����������Դ  

	int clean_resource();

	int get_resource_size();
	long  get_resource_count();


	typename T * get_data_resource(int dataid);
	typename T * get_seq_resource(int currentid, int ref_id);
};


template < typename T, typename T2 >
int source_manag2<T, T2>::init_resourcetab()
{
	//likely()

	count = 0;
	volume = 0;
	memset(data, 0, sizeof(data));
	size = sizeof(data) / sizeof(data[0]);
	lens = 0;
	next_ptr = NULL;

	T * prev = &data[size - 1];
	T * next = &data[0];
	for (int i = 0; i < size; i++)
	{
		data[size - 1 - i].next = next;   //  ����Ĭ�� T �� ���� fake_data �Ľṹ  
		next = &data[size - 1 - i];
		data[i].prev = prev;
		prev = &data[i];
	}
	return 0;
}

template < typename T, typename T2 >
int source_manag2<T, T2>::set_next_ptr(T**  ptr)
{
	next_ptr = (volatile T**)ptr;
	return 0;
}

template < typename T, typename T2 >
volatile T** source_manag2<T, T2>::get_next_ptr()
{
	return next_ptr;
}




//  req �������ö��߳�
//extern void add_log(bool freshen, const char* p, ...);


#if (SUP_MULT_THREAD)

//  ������� ��һ��ʱ����ֻ��һ����Ч���ݵ�������� ��ȷ�ģ�   �������ͬʱ�ֶ����Ч����ʱ �� ���ݵ�˳�����Ԥ�ڣ� ˳���쳣���� ���ݵ�ֵ�쳣 

template < typename T, typename T2 >
typename T* source_manag2<T, T2>::req_resource(T2* addr, int new_volume)
{
	long _new_volume = new_volume;
	long _count;

	if (NULL == addr || NULL == next_ptr)
	{
		add_log(true, "%s%s", "\n req market data pos errer ! ", next_ptr ? " " : "    next_ptr = NULL !");
		return NULL;
	}

	long _volume = volume;

	// ������ ����Ӧ�õ������Դ��� �� ��������Ӧ�ò�һ������ , ��Ϊ Ŀǰ�� 500ms ��������� ֻ��һ�����̵���������Ч�ģ� ���Բ����� ͬʱ�� �ü��� volume ��ֵ ȥ����
	while (_new_volume > _volume)
	{

		if (_volume == InterlockedCompareExchangeAcquire(&volume, _new_volume, _volume))
		{
			while (1)
			{
				_count = count;
				if (_count < size)
				{
					if (volume == new_volume)
					{
						if (_count == InterlockedCompareExchangeAcquire(&count, _count + 1, _count))
						{
							fake_data *ptr = (struct fake_data *)(&data[_count]);
							ptr->D = *addr;
							ptr->prev = (0 == _count && 0 == lens) ? NULL : &data[_count - 1];
							if (ptr->prev)  ((fake_data *)(ptr->prev))->next = ptr;
							*next_ptr = &data[_count];

							return &(data[_count]);
						}
					}
					else
					{
						//  ��ǰ�Ѿ���������   ����
						return NULL;
					}
				}
				else
				{
					if (_count == InterlockedCompareExchangeAcquire(&count, 1, _count))
					{
						fake_data *ptr = (struct fake_data *)(&data[0]);
						ptr->D = *addr;
						ptr->prev = &data[_count - 1];
						if (ptr->prev)  ((fake_data *)(ptr->prev))->next = ptr;
						*next_ptr = &data[0];

						lens = size;
						return &(data[0]);
					}
				}
			}
		}
		_volume = volume;
	}

	return NULL;

}

#else

template < typename T, typename T2 >
typename T* source_manag2<T, T2>::req_resource(T2* addr, int new_volume)
{
	fake_data *ptr = NULL;

	if (NULL == addr || NULL == next_ptr)
	{
		add_log(true, "%s%s", "\n req market data pos errer ! ", next_ptr ? " " : "    next_ptr = NULL !");
		return NULL;
	}


	// ������ ����Ӧ�õ������Դ��� (���)������������Ϊ���̣߳� ��������Ӧ�ò�һ������


	if (new_volume >= volume)
	{
		long _count = count;
		if (_count < size)
		{
			ptr = (struct fake_data *)(&data[_count]);
			ptr->D = *addr;
			//	ptr->prev = (0 == _count && 0 == lens) ? NULL :  &data[_count - 1] ;
			//	if (ptr->prev)  ((fake_data *)(ptr->prev))->next = ptr;
			*next_ptr = (T*)ptr;
			count++;
		}
		else
		{
			ptr = (struct fake_data *)(&data[0]);
			ptr->D = *addr;
			//	ptr->prev = &data[_count - 1];
			//	if (ptr->prev)  ((fake_data *)(ptr->prev))->next = ptr;
			*next_ptr = (T*)ptr;

			lens = size;
			count = 1;
		}
		volume = new_volume;
	}
	return (T *)(ptr);
}

#endif


template < typename T, typename T2 >
typename T * source_manag2<T, T2>::req_resource(int new_volume)  // ����������Դ  
{

	fake_data *ptr = NULL;

	if (  NULL == next_ptr)
	{
		add_log(true, "%s%s", "\n req market data pos errer ! ", next_ptr ? " next_ptr== NULL " : "    next_ptr = NULL !");
		return NULL;
	}


	// ������ ����Ӧ�õ������Դ��� (���)������������Ϊ���̣߳� ��������Ӧ�ò�һ������


	if (new_volume >= volume)
	{
		long _count = count;
		if (_count < size)
		{
			ptr = (struct fake_data *)(&data[_count]);
			*next_ptr = (T*)ptr;
			count++;
		}
		else
		{
			ptr = (struct fake_data *)(&data[0]);
			*next_ptr = (T*)ptr;
			lens = size;
			count = 1;
		}
		volume = new_volume;
	}
	return (T *)(ptr);
}









//  �ڹ̶���ʼ�������� ���ػ�����ʷ���ݣ�  ref_id ��ʾ���ݴ����� 0�� ��ʾ��ǰ �� 1����ʾǰһ��

template < typename T, typename T2 >
typename T * source_manag2<T, T2>::get_seq_resource(int currentid, int ref_id)
{
	if (currentid >= ref_id)     return &(data[currentid - ref_id]);
	else return  &(data[size + currentid - ref_id]);
}


template < typename T, typename T2 >
int source_manag2<T, T2>::clean_resource()
{
	count = 0;
	volume = 0;
	lens = 0;

	//	memset(data, 0, sizeof(0));
	return 0;
}



/*
template<typename T>
typename T * source_manag2<T>::get_seq_resource(T *addr, int ref_id)
{
if (currentid >= ref_id)     return &(data[currentid - ref_id]);
else return  &(data[size + currentid - ref_id]);
}
*/

template < typename T, typename T2 >
typename T * source_manag2<T, T2>::get_data_resource(int dataid)
{
	if (dataid < size)
	{
		return  &(data[dataid]);
	}
	else
		return NULL;
}

template < typename T, typename T2 >
long source_manag2<T, T2>::get_resource_count()
{
	return count;
}

template < typename T, typename T2 >
int	source_manag2<T, T2>::get_resource_size()
{
	return size;
}












