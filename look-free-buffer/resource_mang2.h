/*  说明 ：此类实现了一个资源分配器，分为 单线程和多线程版本
多线程：基于无锁编程实现了 支持多线程的 资源分配器
功能: 实现了一个 资源的分配器，用于资源的快速申请和释放
资源的申请可以多线程执行

单线程： 一个环形的 资源池 顺序获得资源 ， 循环申请使用
此类中的资源属于 循环使用的类型 ，  所以不存在显示的是回收操作 ,
另外 T数据结构体 使用 单链表的形式串起来了 ， 所以 元数据 结构中不许按要求构造结构体。

// update 190313 ： 由于是 顺序资源， 所以 初始化了 next 和 prev 指针 ， 清除的时候只操作 实际的数据D ,  在获取资源时 也只存储资源， 不做其他操作


// 当前使用范围  ：  tick 数据存储 


*/

#pragma once 
#include<iostream>  
#include<Windows.h>  

using namespace std;


//  support multly thread
#define SUP_MULT_THREAD  FALSE   //  tick 支持多线程 


#undef  RESOURCE_SZIE2
#define RESOURCE_SZIE2 30    // 社sing缓存数量




template < typename T, typename T2 >

class source_manag2
{
	struct fake_data   //  fake——data 的结构是 T的结构的头部 ， 必须结构一致，字节对齐  
	{
		void *prev;
		T2	  D;
		void *next;
	};


private:

	volatile long count;
	volatile long volume;
	volatile T**  next_ptr;  //  用于 标记最新数据的 地址  
	int lens;

	T	 data[RESOURCE_SZIE2];
	int size;



public:

	source_manag2(){};
	~source_manag2(){};

	int init_resourcetab();
	int set_next_ptr(T**  ptr);
	volatile T** get_next_ptr();


	typename T * req_resource(T2* addr, int new_volume);  // 申请资源 并存储数据

	typename T * req_resource(int new_volume);  // 仅仅申请资源  

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
		data[size - 1 - i].next = next;   //  这里默认 T 中 符合 fake_data 的结构  
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




//  req 函数适用多线程
//extern void add_log(bool freshen, const char* p, ...);


#if (SUP_MULT_THREAD)

//  这个函数 在一定时间内只有一个有效数据的情况下是 正确的，   如果出现同时又多个有效数据时 ， 数据的顺序会会非预期， 顺序异常后导致 数据的值异常 

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

	// 这里有 金融应用的特殊性存在 ， 用于其他应用不一定可行 , 因为 目前是 500ms 多个进程中 只有一个进程的数据是有效的， 所以不会有 同时有 好几种 volume 的值 去更新
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
						//  当前已经不是最新   丢弃
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


	// 这里有 金融应用的特殊性存在 (间隔)，且行情推送为单线程， 用于其他应用不一定可行


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
typename T * source_manag2<T, T2>::req_resource(int new_volume)  // 仅仅申请资源  
{

	fake_data *ptr = NULL;

	if (  NULL == next_ptr)
	{
		add_log(true, "%s%s", "\n req market data pos errer ! ", next_ptr ? " next_ptr== NULL " : "    next_ptr = NULL !");
		return NULL;
	}


	// 这里有 金融应用的特殊性存在 (间隔)，且行情推送为单线程， 用于其他应用不一定可行


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









//  在固定起始点的情况下 返回回溯历史数据，  ref_id 表示回溯次数， 0： 表示当前 ， 1：表示前一个

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












