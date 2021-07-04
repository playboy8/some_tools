/*  说明 ：   本算法基于无锁编程实现了 支持多线程的 资源分配器
功能: 实现了一个 资源的分配器，用于资源的快速申请和释放
资源的申请可以多线程执行
资源的回收 申请线程只能标记 ， 实际的回收操作由固定的维护线程完成
所以 在实现运用中 关键要注意 确定申请线程 和 维护线程 ， 维护线程的操作频率 过低 会导致 资源枯竭
*/

#pragma once 
#include<iostream>  
#include<Windows.h>  

#include "Lock.h"

#define USE_SOURCE_MANAGE3  FALSE    //  TRUE //  是否使用 source_manag 类模板 

//   USE_SOURCE_MANAGE3 为TRUE 时， 订单的 挂单计数还存在问题， 待调试  20200715 





////////////////////////////////////////////  public  //////////////////////////////////////////////////
using namespace std;


#include "std_quant_define.h"
#define container_of(ptr, type, member)  ( (type*)((char*)ptr - offsetof(type, member)) )


#define USE_USERID_IN_ORDER  TRUE    //  定义 是否在资源起始位置 使用 unsigned int index 标记。



#if !USE_SOURCE_MANAGE3   
///////////////////////////////////////////////source_manag   模板类  /////////////////////////////////////////////

#define _state_init   0    //  资源初始态
#define _state_used   1		// 资源已使用
#define _state_abund   2    // 资源已弃用 ，待回收




using namespace LockTool;

template < typename T >

class source_manag
{
private:

	volatile unsigned  long count;
	unsigned int size;
	int  index[ORDER_RESOURCE_SZIE];
	char used[ORDER_RESOURCE_SZIE];
	T	 data[ORDER_RESOURCE_SZIE];

	bool(*judge_mathod)(T*);  // 在清理数据时刻 判断数据是否失效 
	
	CLockAtom ealock;

public:

	source_manag();

	~source_manag();

	int init_resourcetab();


	// 注意： 请求资源得到的 资源 除了 头部index ， 其他数据是未经初始化的 ， 需要自行初始化 
	typename T * req_resource();  // 请求资源
	typename T * req_mult_resource(unsigned int len);  // 批量请求资源
	typename T * req_resource(unsigned int id);  // 请求 index id的 资源 

	int use_resource(T *addr);   // 标记资源被使用 



	int free_resource(T *addr);   // 标记资源被废弃, 以资源地址为索引
	int free_resource(unsigned int indexid); // 标记资源被废弃， 以 indexid 为索引
	int free_resource2(unsigned int indexid); // 标记资源被废弃， 以 dataid 为索引


	int free_resource_real();   // 清理并重置资源 



	typename T * get_resource(unsigned int indexid);  //  获取特定index id的 资源

	long  get_resource_count();   //获取已使用资源个数

	int dataclear(int id);  // 重置整个资源 


	//  another view mathod

	int get_resource_size();   // 获取资源容量 
	typename T * get_data_resource(unsigned int dataid);  //  根据 dataid 获取资源地址 

	int get_data_index(T *addr);  //  根据数据地址 获取 dataid  

	void* set_judge_mathod( bool (*func)(T*) );  // 设置判断函数 （未用）

};



template < typename T >
 source_manag<T>::source_manag()
{
	init_resourcetab();
}
 template < typename T >
 source_manag<T>::~source_manag()
{
}



template < typename T >
int source_manag<T>::init_resourcetab()
{
	int i;
	count = 0;
	memset(data, 0, sizeof(data));
	memset(used, _state_init, sizeof(used));
	for (i = 0; i < sizeof(index) / sizeof(index[0]); i++)
	{
		index[i] = i;
		// used[i] = _state_init;  //  必须放在外面 
	}
	size = sizeof(index) / sizeof(index[0]) ;
	return 0;
}

template < typename T >
void* source_manag<T>::set_judge_mathod(bool(*func)(T*))
{
	judge_mathod = func;
	return judge_mathod;
}

 

//  req 函数适用多线程
//  extern void add_log(bool freshen, const char* p, ...);

template<typename T>
typename T* source_manag<T>::req_resource()
{
	unsigned long _count;

	_count = count;
	if (_count < size)
	{
		do {

			if (_count == InterlockedCompareExchangeAcquire(&count, _count + 1, _count))
			{
				int id = index[_count ];
				T* p = &(data[id]);
#if USE_USERID_IN_ORDER
				int *pint = (int*)(p);
				*pint = id;
#endif

				return p ;
			}

			_count = count;

		} while (_count < size);
	}

	//	add_log(true, "%s", "\n req source_manag<>  error ! " );
	return NULL;
}


template<typename T>
typename T* source_manag<T>::req_mult_resource(unsigned int len)    // 批量请求资源
{
	unsigned long _count;

	_count = count;
	if (_count + len < size)
	{
		do {

			if (_count == InterlockedCompareExchangeAcquire(&count, _count + len, _count))
			{
				
				for (int i = 0; i < len; i++)
				{
					int id = index[_count + i];
					T* p = &(data[id]); //T* p = &(data[index[_count + i]]);				
#if USE_USERID_IN_ORDER
					int *pint = (int*)(p);
					*pint = id;
#endif
				}
				return &(data[index[_count ]]); //p;
			}

			_count = count;

		} while (_count + len < size);
	}

	//	add_log(true, "%s", "\n req source_manag<>  error ! " );
	return NULL;
}

//  id: 数据索引位置 ， 从0 开始 
// 本函数功能是 取 特定id位置的 资源， 需要满足 id >= count 这个条件, 即资源未被占用   

template<typename T>
typename T* source_manag<T>::req_resource(unsigned int id)
{
	unsigned long _count;

	_count = count;
	if ( _count <= id && id < size)
	{
		do {

			if (_count == InterlockedCompareExchangeAcquire(&count, id + 1, _count))
			{
				T* p = &(data[index[id]]);
#if USE_USERID_IN_ORDER
				int *pint = (int*)(p);
				*pint = index[id];
#endif

				return p;
			}

			_count = count;

		} while (_count <= id && id < size);
	}

	//	add_log(true, "%s", "\n req source_manag<>  error ! " );
	return NULL;
}

//  req线程操作  
template<typename T>
int source_manag<T>::use_resource(T *addr)  //    标记资源状态到 使用状态
{
	return 0; 
#if 0

	//int indexid = ((char*)addr - (char*)data) / sizeof(data[0]);
	int indexid = *((int*)addr);      //  采用标记值

	if (indexid >= 0 && indexid < size)
	{
		used[indexid] = _state_used;
		return 0;
	}
	else
		return -1;
#endif 
}


// 该函数只能 维护线程运行 
// 用于 申请线程 释放资源     标记函数   
template<typename T>
int source_manag<T>::free_resource(T *addr)
{
	//int indexid = ((char*)addr - (char*)data) / sizeof(data[0]);

	int indexid = *((int*)addr);      //  采用标记值

	if (indexid >= 0 && indexid < size)
	{

		used[indexid] = _state_abund;
	//	memset(&(data[indexid]), 0, sizeof(data[0]));    // 资源在每次使用时  单独初始化 ， 这里置零 会导致 界面不能更新  撤单 等信息 

		return 0;
	}
	else  return -1;
}

//   该函数只能 维护线程运行 
template<typename T>
int source_manag<T>::free_resource(unsigned int indexid)
{
	if (indexid >= 0 && indexid < size)
	{

		used[index[indexid]] = _state_abund;
	//	memset(&(data[index[indexid]]), 0, sizeof(data[0])); // 资源在每次使用时  单独初始化 ， 这里置零 会导致 界面不能更新  撤单 等信息 

#ifdef STD_LOG_SWITCH
		//	add_log(true, "%s%d", "\n res_free_id = ", indexid);
#endif
		return 0;
	}
	else
	{
#ifdef STD_LOG_SWITCH
		//	add_log(false, "%s", "\n res_free_id = error  !  ");
#endif
		return -1;
	}
}


template<typename T>
int source_manag<T>::free_resource2(unsigned int indexid)  // 标记资源被废弃， 以 dataid 为索引
{
	if (indexid >= 0 && indexid < size)
	{

		used[indexid] = _state_abund;
	//	memset(&(data[indexid]), 0, sizeof(data[0]));

#ifdef STD_LOG_SWITCH
		//	add_log(true, "%s%d", "\n res_free_id = ", indexid);
#endif
		return 0;
	}
	else
	{
#ifdef STD_LOG_SWITCH
		//	add_log(false, "%s", "\n res_free_id = error  !  ");
#endif
		return -1;
	}





}


//  该函数只能 维护线程运行 
template<typename T>
int source_manag<T>::free_resource_real()
{
	CLockAtomRegion lock(&ealock);  //加锁 执行 

	int temp, i, j;
	bool change = false;

	long  _count = count;
	int start_i = 0, start_j = _count - 1;


	if (((_count >> 1) + _count) >= size) //   大于67%  
	{

		for (i = start_i, j = start_j; _count > 0 && i < _count && i <= start_j; i++)
		{
			if (_state_abund <= used[index[i]])
			{
				change = true;
				for (j = start_j; j > i; j--)
				{
					if (_state_abund > used[index[j]])
					{
						start_i = i + 1;
						start_j = j - 1;
						temp = index[i];
						index[i] = index[j];
						index[j] = temp;
						used[temp] = _state_init;
						break;
					}
					else
					{
						//  not found 
						used[index[j]] = _state_init;
					}
				}
				if (start_j != j - 1)
				{
					used[index[i]] = _state_init;
					start_i = i;
					start_j = j;
					break;
				}
			}
			else start_i = i + 1;
		}


		if (true == change)
		{
		free_resource_redo1:
			if (_count == InterlockedCompareExchangeAcquire(&count, start_i, _count))
			{
				//	add_log(true, "%s%d", "\n res_clean_count= ", start_i);

				return start_i;
			}
			else
			{
				// count  一定是增加了 , 因为count只能增加  


				long last_cout = _count;
				_count = count;

				for (i = _count - 1; i > last_cout - 1; i--)
				{
					temp = index[i];
					index[i] = index[start_i];
					index[start_i] = temp;
					used[temp] = _state_init;
					start_i++;
				}
				goto free_resource_redo1;
			}
		}
	}
	return -1;
}


//  需要在资源 有效的 情况下 返回资源 
template<typename T>
typename T * source_manag<T>::get_resource(unsigned int indexid)
{
	if (indexid < count)
		return   (_state_abund != used[index[indexid]]) ? (&(data[index[indexid]])) : NULL;
	else
		return NULL;
}

template<typename T>
typename T * source_manag<T>::get_data_resource(unsigned int dataid)
{
	if (dataid < size)
	{
		return  (_state_abund  != used[dataid]) ? &(data[dataid]) : NULL;
	}
	else
		return NULL;
}

template<typename T>
long source_manag<T>::get_resource_count()
{
	return count;
}

template<typename T>
int	source_manag<T>::get_resource_size()
{
	return size;
}

template<typename T>
int source_manag<T>::get_data_index(T *addr)
{
	if (addr >= data  && addr <= &(data[size - 1]))
	{
		return   ((char*)addr - (char*)data) / sizeof(data[0]);
	}
	return -1;
}


// 归还资源前必须调用   	MemoryBarrier();

template<typename T>
int source_manag<T>::dataclear(int id)
{
	memset(&(data[index[id]]), 0, sizeof(data[0]));
	used[index[id]] = 0;
	return 0;
}





#else
////////////////////////////////   source_manag  模板类 ////////////////////////////////////////




#define _state_init   1    //  资源初始态,  bit为 1  表示该位置可用
#define _state_used   0		// 资源已使用， bit为 0 表示该位置已经使用， 不可用 


#define container_of(ptr, type, member)  ( (type*)((char*)ptr - offsetof(type, member)) )



//获取64位 无符号数据 第一个bit1 的位置  ， 由低到高    bit0   bit1      .......bit63, 如果输出 64 表示 没有bit位为1。
inline unsigned int get_first_bit1(unsigned __int64 index)
{

	//将第一个为1位的低位都置1，其它位都置0
	index = (index - 1) & (~index);
	//得到有多少为1的位
	index = (index & 0x5555555555555555) + ((index >> 1) & 0x5555555555555555);
	index = (index & 0x3333333333333333) + ((index >> 2) & 0x3333333333333333);
	index = (index & 0x0F0F0F0F0F0F0F0F) + ((index >> 4) & 0x0F0F0F0F0F0F0F0F);
	index = (index & 0x00FF00FF00FF00FF) + ((index >> 8) & 0x00FF00FF00FF00FF);
	index = (index & 0x0000FFFF0000FFFF) + ((index >> 16) & 0x0000FFFF0000FFFF);
	index = (index & 0x00000000FFFFFFFF) + ((index >> 32) & 0x00000000FFFFFFFF);

	//得到位数,如果为64则表示全0
	return  unsigned int(index & 0x7f);
}

inline unsigned __int64 bit_test(unsigned __int64 val, unsigned int id)
{
	return  (0x01 << id) & val;
}


template < typename T >

class source_manag
{
private:
	unsigned int size;
	volatile __int64 bitmap;
	T	 data[64];

public:

	source_manag();

	~source_manag();

	int init_resourcetab();

	typename T * req_resource();  // 请求资源
	typename T * req_resource(unsigned int id);  // 请求 index id的 资源 
	int use_resource(T *addr);   // 标记资源被使用 

	int free_resource(T *addr);   // 标记资源被废弃, 以资源地址为索引
	int free_resource(unsigned int indexid); // 标记资源被废弃， 以 indexid 为索引

	int free_resource_real();   // 清理并重置资源

	typename T * get_resource(unsigned int indexid);  //  获取特定index id的数据句柄
	typename T * get_data_resource(unsigned int dataid);  //  根据 dataid 获取资源地址 

	unsigned int  get_resource_count();   //获取已使用资源个数

	int get_resource_size();   // 获取资源容量 


};



template < typename T >
source_manag<T>::source_manag()
{
	size = sizeof(bitmap)* 8;
	init_resourcetab();
}
template < typename T >
source_manag<T>::~source_manag()
{
}



template < typename T >
int source_manag<T>::init_resourcetab()
{
	__int64 temp = bitmap;
	unsigned __int64 temp_new = 0;
	temp_new = __int64(~temp_new);
	while (temp != InterlockedCompareExchangeAcquire64(&bitmap, temp_new, temp))
		temp = bitmap;

	return 0;
}



//  req 函数适用多线程
//  extern void add_log(bool freshen, const char* p, ...);

template<typename T>
typename T* source_manag<T>::req_resource()
{
	 __int64 temp = bitmap;
	 __int64 new_temp = 0;
	unsigned int pos = get_first_bit1(temp);

	if (pos < 64)
	{
		do {

			new_temp = (temp & (~(0x01ULL << pos)));

			if (temp == InterlockedCompareExchangeAcquire64(&bitmap, new_temp, temp))
			{
				T* p = &(data[pos]);
#if USE_USERID_IN_ORDER
				unsigned int *pint = (unsigned int*)(p);
				*pint = pos;
#endif

				return p;
			}

			temp = bitmap;
			pos = get_first_bit1(temp);

		} while (pos < 64);
	}

	add_log(true, "%s", "\n[error]: req source_manag<>  Faild  ! ");
	return NULL;
}


//  id: 数据索引位置 ， 从0 开始 
// 本函数功能是 取 特定id位置的 资源， 需要满足 id < size 这个条件 

template<typename T>
typename T* source_manag<T>::req_resource(unsigned int id)
{
	__int64 temp = bitmap;
	__int64 new_temp = 0;

	if (id < size && (0 != ((0x01ULL << id) & temp)))
	{
		new_temp = (temp & (~(0x01ULL << id)));
		if (temp == InterlockedCompareExchangeAcquire64(&bitmap, new_temp, temp))
		{
			T* p = &(data[id]);
#if USE_USERID_IN_ORDER
			unsigned int *pint = (unsigned int*)(p);
			*pint = id;
#endif

			return p;
		}

	}

	add_log(true, "%s", "\n[error]: req source_manag<>  Faild  ! ");
	return NULL;
}


// 该函数只能 维护线程运行 
// 用于 申请线程 释放资源     标记函数   
template<typename T>
int source_manag<T>::free_resource(T *addr)
{
	//int indexid = ((char*)addr - (char*)data) / sizeof(data[0]);

	unsigned int indexid = *((unsigned int*)addr);      //  采用标记值, 该结构的第一个成员为 索引值
	unsigned __int64 temp = bitmap;
	unsigned __int64 new_temp;


	if (indexid < size && (0 == (temp &(0x01ULL << indexid))))
	{
		new_temp = temp | (0x01ULL << indexid);
		while (temp != InterlockedCompareExchangeAcquire64(&bitmap, new_temp, temp))
		{
			temp = bitmap;
			new_temp = temp | (0x01ULL << indexid);

			if (0 != (temp &(0x01ULL << indexid)))  return -1;
		}
		return 0;
	}
	else
	{
		//add_log(true, "%s%d", "\n[Error]: free_resource() failed !   res_free_id = ", indexid);
		return -1;
	}
}

//   该函数只能 维护线程运行 
template<typename T>
int source_manag<T>::free_resource(unsigned int indexid)
{

	unsigned __int64 temp = bitmap;
	unsigned __int64 new_temp;

	if (indexid < size && (0 == temp &(0x01ULL << indexid)))
	{
		new_temp = temp | (0x01ULL << indexid);
		while (temp != InterlockedCompareExchangeAcquire64(&bitmap, new_temp, temp))
		{
			temp = bitmap;
			new_temp = temp | (0x01ULL << indexid);

			if (0 != (temp &(0x01ULL << indexid)))  return -1;
		}
		return 0;
	}
	else
	{
		add_log(true, "%s%d", "\n[Error]: free_resource() failed !   res_free_id = ", indexid);
		return -1;
	};
}
 
template<typename T>
int source_manag<T>::free_resource_real()   // 清理并重置资源
{
	return 0;
}

//  需要在资源 有效的 情况下 返回资源 
template<typename T>
typename T * source_manag<T>::get_resource(unsigned int indexid)
{
	__int64 temp = bitmap;
	if (indexid < size && (0 == (temp &(0x01ULL << indexid))))
		return   &data[indexid];
	else
		return NULL;
}


template<typename T>
typename T * source_manag<T>::get_data_resource(unsigned int dataid)  //  根据 dataid 获取资源地址 
{
	__int64 temp = bitmap;
	bool ret1 = (0 == (temp &(0x01ULL << dataid)));
	bool ret2 = (dataid < size);
	if (ret1 && ret2 )
		return   &data[dataid];
	else
		return NULL;


}


template<typename T>
unsigned int  source_manag<T>::get_resource_count()
{
	__int64 index = bitmap;
	index = (index & 0x5555555555555555) + ((index >> 1) & 0x5555555555555555);
	index = (index & 0x3333333333333333) + ((index >> 2) & 0x3333333333333333);
	index = (index & 0x0F0F0F0F0F0F0F0F) + ((index >> 4) & 0x0F0F0F0F0F0F0F0F);
	index = (index & 0x00FF00FF00FF00FF) + ((index >> 8) & 0x00FF00FF00FF00FF);
	index = (index & 0x0000FFFF0000FFFF) + ((index >> 16) & 0x0000FFFF0000FFFF);
	index = (index & 0x00000000FFFFFFFF) + ((index >> 32) & 0x00000000FFFFFFFF);

	//得到位数,如果为64则表示全0
	return  unsigned int(index & 0x7f);

}

template<typename T>
int	source_manag<T>::get_resource_size()
{
	return size;
}


template<typename T>
int	source_manag<T>::use_resource(T *addr)    // 标记资源被使用 
{
	return 0;
}

#endif








