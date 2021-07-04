/*  ˵�� ��   ���㷨�����������ʵ���� ֧�ֶ��̵߳� ��Դ������
����: ʵ����һ�� ��Դ�ķ�������������Դ�Ŀ���������ͷ�
��Դ��������Զ��߳�ִ��
��Դ�Ļ��� �����߳�ֻ�ܱ�� �� ʵ�ʵĻ��ղ����ɹ̶���ά���߳����
���� ��ʵ�������� �ؼ�Ҫע�� ȷ�������߳� �� ά���߳� �� ά���̵߳Ĳ���Ƶ�� ���� �ᵼ�� ��Դ�ݽ�
*/

#pragma once 
#include<iostream>  
#include<Windows.h>  

#include "Lock.h"

#define USE_SOURCE_MANAGE3  FALSE    //  TRUE //  �Ƿ�ʹ�� source_manag ��ģ�� 

//   USE_SOURCE_MANAGE3 ΪTRUE ʱ�� ������ �ҵ��������������⣬ ������  20200715 





////////////////////////////////////////////  public  //////////////////////////////////////////////////
using namespace std;


#include "std_quant_define.h"
#define container_of(ptr, type, member)  ( (type*)((char*)ptr - offsetof(type, member)) )


#define USE_USERID_IN_ORDER  TRUE    //  ���� �Ƿ�����Դ��ʼλ�� ʹ�� unsigned int index ��ǡ�



#if !USE_SOURCE_MANAGE3   
///////////////////////////////////////////////source_manag   ģ����  /////////////////////////////////////////////

#define _state_init   0    //  ��Դ��ʼ̬
#define _state_used   1		// ��Դ��ʹ��
#define _state_abund   2    // ��Դ������ ��������




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

	bool(*judge_mathod)(T*);  // ����������ʱ�� �ж������Ƿ�ʧЧ 
	
	CLockAtom ealock;

public:

	source_manag();

	~source_manag();

	int init_resourcetab();


	// ע�⣺ ������Դ�õ��� ��Դ ���� ͷ��index �� ����������δ����ʼ���� �� ��Ҫ���г�ʼ�� 
	typename T * req_resource();  // ������Դ
	typename T * req_mult_resource(unsigned int len);  // ����������Դ
	typename T * req_resource(unsigned int id);  // ���� index id�� ��Դ 

	int use_resource(T *addr);   // �����Դ��ʹ�� 



	int free_resource(T *addr);   // �����Դ������, ����Դ��ַΪ����
	int free_resource(unsigned int indexid); // �����Դ�������� �� indexid Ϊ����
	int free_resource2(unsigned int indexid); // �����Դ�������� �� dataid Ϊ����


	int free_resource_real();   // ����������Դ 



	typename T * get_resource(unsigned int indexid);  //  ��ȡ�ض�index id�� ��Դ

	long  get_resource_count();   //��ȡ��ʹ����Դ����

	int dataclear(int id);  // ����������Դ 


	//  another view mathod

	int get_resource_size();   // ��ȡ��Դ���� 
	typename T * get_data_resource(unsigned int dataid);  //  ���� dataid ��ȡ��Դ��ַ 

	int get_data_index(T *addr);  //  �������ݵ�ַ ��ȡ dataid  

	void* set_judge_mathod( bool (*func)(T*) );  // �����жϺ��� ��δ�ã�

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
		// used[i] = _state_init;  //  ����������� 
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

 

//  req �������ö��߳�
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
typename T* source_manag<T>::req_mult_resource(unsigned int len)    // ����������Դ
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

//  id: ��������λ�� �� ��0 ��ʼ 
// ������������ ȡ �ض�idλ�õ� ��Դ�� ��Ҫ���� id >= count �������, ����Դδ��ռ��   

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

//  req�̲߳���  
template<typename T>
int source_manag<T>::use_resource(T *addr)  //    �����Դ״̬�� ʹ��״̬
{
	return 0; 
#if 0

	//int indexid = ((char*)addr - (char*)data) / sizeof(data[0]);
	int indexid = *((int*)addr);      //  ���ñ��ֵ

	if (indexid >= 0 && indexid < size)
	{
		used[indexid] = _state_used;
		return 0;
	}
	else
		return -1;
#endif 
}


// �ú���ֻ�� ά���߳����� 
// ���� �����߳� �ͷ���Դ     ��Ǻ���   
template<typename T>
int source_manag<T>::free_resource(T *addr)
{
	//int indexid = ((char*)addr - (char*)data) / sizeof(data[0]);

	int indexid = *((int*)addr);      //  ���ñ��ֵ

	if (indexid >= 0 && indexid < size)
	{

		used[indexid] = _state_abund;
	//	memset(&(data[indexid]), 0, sizeof(data[0]));    // ��Դ��ÿ��ʹ��ʱ  ������ʼ�� �� �������� �ᵼ�� ���治�ܸ���  ���� ����Ϣ 

		return 0;
	}
	else  return -1;
}

//   �ú���ֻ�� ά���߳����� 
template<typename T>
int source_manag<T>::free_resource(unsigned int indexid)
{
	if (indexid >= 0 && indexid < size)
	{

		used[index[indexid]] = _state_abund;
	//	memset(&(data[index[indexid]]), 0, sizeof(data[0])); // ��Դ��ÿ��ʹ��ʱ  ������ʼ�� �� �������� �ᵼ�� ���治�ܸ���  ���� ����Ϣ 

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
int source_manag<T>::free_resource2(unsigned int indexid)  // �����Դ�������� �� dataid Ϊ����
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


//  �ú���ֻ�� ά���߳����� 
template<typename T>
int source_manag<T>::free_resource_real()
{
	CLockAtomRegion lock(&ealock);  //���� ִ�� 

	int temp, i, j;
	bool change = false;

	long  _count = count;
	int start_i = 0, start_j = _count - 1;


	if (((_count >> 1) + _count) >= size) //   ����67%  
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
				// count  һ���������� , ��Ϊcountֻ������  


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


//  ��Ҫ����Դ ��Ч�� ����� ������Դ 
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


// �黹��Դǰ�������   	MemoryBarrier();

template<typename T>
int source_manag<T>::dataclear(int id)
{
	memset(&(data[index[id]]), 0, sizeof(data[0]));
	used[index[id]] = 0;
	return 0;
}





#else
////////////////////////////////   source_manag  ģ���� ////////////////////////////////////////




#define _state_init   1    //  ��Դ��ʼ̬,  bitΪ 1  ��ʾ��λ�ÿ���
#define _state_used   0		// ��Դ��ʹ�ã� bitΪ 0 ��ʾ��λ���Ѿ�ʹ�ã� ������ 


#define container_of(ptr, type, member)  ( (type*)((char*)ptr - offsetof(type, member)) )



//��ȡ64λ �޷������� ��һ��bit1 ��λ��  �� �ɵ͵���    bit0   bit1      .......bit63, ������ 64 ��ʾ û��bitλΪ1��
inline unsigned int get_first_bit1(unsigned __int64 index)
{

	//����һ��Ϊ1λ�ĵ�λ����1������λ����0
	index = (index - 1) & (~index);
	//�õ��ж���Ϊ1��λ
	index = (index & 0x5555555555555555) + ((index >> 1) & 0x5555555555555555);
	index = (index & 0x3333333333333333) + ((index >> 2) & 0x3333333333333333);
	index = (index & 0x0F0F0F0F0F0F0F0F) + ((index >> 4) & 0x0F0F0F0F0F0F0F0F);
	index = (index & 0x00FF00FF00FF00FF) + ((index >> 8) & 0x00FF00FF00FF00FF);
	index = (index & 0x0000FFFF0000FFFF) + ((index >> 16) & 0x0000FFFF0000FFFF);
	index = (index & 0x00000000FFFFFFFF) + ((index >> 32) & 0x00000000FFFFFFFF);

	//�õ�λ��,���Ϊ64���ʾȫ0
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

	typename T * req_resource();  // ������Դ
	typename T * req_resource(unsigned int id);  // ���� index id�� ��Դ 
	int use_resource(T *addr);   // �����Դ��ʹ�� 

	int free_resource(T *addr);   // �����Դ������, ����Դ��ַΪ����
	int free_resource(unsigned int indexid); // �����Դ�������� �� indexid Ϊ����

	int free_resource_real();   // ����������Դ

	typename T * get_resource(unsigned int indexid);  //  ��ȡ�ض�index id�����ݾ��
	typename T * get_data_resource(unsigned int dataid);  //  ���� dataid ��ȡ��Դ��ַ 

	unsigned int  get_resource_count();   //��ȡ��ʹ����Դ����

	int get_resource_size();   // ��ȡ��Դ���� 


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



//  req �������ö��߳�
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


//  id: ��������λ�� �� ��0 ��ʼ 
// ������������ ȡ �ض�idλ�õ� ��Դ�� ��Ҫ���� id < size ������� 

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


// �ú���ֻ�� ά���߳����� 
// ���� �����߳� �ͷ���Դ     ��Ǻ���   
template<typename T>
int source_manag<T>::free_resource(T *addr)
{
	//int indexid = ((char*)addr - (char*)data) / sizeof(data[0]);

	unsigned int indexid = *((unsigned int*)addr);      //  ���ñ��ֵ, �ýṹ�ĵ�һ����ԱΪ ����ֵ
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

//   �ú���ֻ�� ά���߳����� 
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
int source_manag<T>::free_resource_real()   // ����������Դ
{
	return 0;
}

//  ��Ҫ����Դ ��Ч�� ����� ������Դ 
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
typename T * source_manag<T>::get_data_resource(unsigned int dataid)  //  ���� dataid ��ȡ��Դ��ַ 
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

	//�õ�λ��,���Ϊ64���ʾȫ0
	return  unsigned int(index & 0x7f);

}

template<typename T>
int	source_manag<T>::get_resource_size()
{
	return size;
}


template<typename T>
int	source_manag<T>::use_resource(T *addr)    // �����Դ��ʹ�� 
{
	return 0;
}

#endif








