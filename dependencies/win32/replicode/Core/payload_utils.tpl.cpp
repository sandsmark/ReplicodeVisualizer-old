//	payload_utils.tpl.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Eric Nivel
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Eric Nivel nor the
//     names of their contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
//	THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#include	<string.h>

namespace	mBrane{
	namespace	sdk{
		namespace	payloads{
/*
			template<typename	T>	void	*Storage<T>::New(uint32	size){	
				
				uint32	ns;
				return	new(size,ns)	Storage();
			}
			
			template<typename	T>	void	*Storage<T>::operator	new(size_t	s,uint32	size,uint32	&normalizedSize){

				Storage<T>	*storage=(Storage<T>	*)Memory::GetDynamic(size)->alloc(normalizedSize);
				storage->normalizedSize=normalizedSize;
				storage->count=(size-sizeof(Storage<T>))/sizeof(T);
				return	storage;
			}
			
			template<typename	T>	void	Storage<T>::operator	delete(void	*storage){

				Memory::GetDynamic(((Storage<T>	*)storage)->getNormalizedSize())->dealloc(storage);
			}
			
			template<typename	T>	inline	Storage<T>::Storage(){	
				
				this->_metaData=Storage<T>::_MetaData;
				this->data=((T	*)(((uint8	*)this)+offsetof(Storage<T>,data)+sizeof(T	*)));
			}

			template<typename	T>	Storage<T>::~Storage(){
			}

			template<typename	T>	inline	uint32	Storage<T>::getNormalizedSize(){

				return	normalizedSize;
			}

			template<typename	T>	inline	void	Storage<T>::setNormalizedSize(uint32	size){	
				
				normalizedSize=size;
			}
				
			template<typename	T>	T	&Storage<T>::operator	[](uint32	i){	
				
				return	data[i];
			}
			
			template<typename	T>	size_t	Storage<T>::size()	const{	
				
				return	sizeof(Storage<T>)+sizeof(T)*count;
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			template<typename	T>	void	*Storage<P<T> >::New(uint32	size){	
				
				uint32	ns;
				return	new(size,ns)	Storage();
			}
			
			template<typename	T>	void	*Storage<P<T> >::operator	new(size_t	s,uint32	size,uint32	&normalizedSize){

				Storage<T>	*storage=(Storage<P<T> >	*)Memory::GetDynamic(size)->alloc(normalizedSize);
				storage->normalizedSize=normalizedSize;
				storage->count=(size-sizeof(Storage<P<T> >))/sizeof(T);
				return	storage;
			}
			
			template<typename	T>	void	Storage<P<T> >::operator	delete(void	*storage){

				Memory::GetDynamic(((Storage<P<T> >	*)storage)->getNormalizedSize())->dealloc(storage);
			}
			
			template<typename	T>	inline	Storage<P<T> >::Storage(){	
				
				this->_metaData=Storage<P<T> >::_MetaData;
				this->data=((P<T>	*)(((uint8	*)this)+offsetof(Storage<P<T> >,data)+sizeof(P<T>	*)));
			}

			template<typename	T>	Storage<P<T> >::~Storage(){
			}

			template<typename	T>	inline	uint32	Storage<P<T> >::getNormalizedSize(){

				return	normalizedSize;
			}

			template<typename	T>	inline	void	Storage<P<T> >::setNormalizedSize(uint32	size){	
				
				normalizedSize=size;
			}
				
			template<typename	T>	P<T>	&Storage<P<T> >::operator	[](uint32	i){	
				
				return	data[i];
			}
			
			template<typename	T>	size_t	Storage<P<T> >::size()	const{	
				
				return	sizeof(Storage<P<T> >)+sizeof(P<T>)*count;
			}
				
			template<typename	T>	uint16	Storage<P<T> >::ptrCount()	const{
				
				return	count;
			}
			
			template<typename	T>	__Payload	*Storage<P<T> >::getPtr(uint16	i)	const{
				
				return	data[i];
			}
			
			template<typename	T>	void	Storage<P<T> >::setPtr(uint16	i,__Payload	*p){
				
				data[i]=(T	*)p;
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			template<typename	T>	Array<T>::Array():_data(NULL),_maxCount(0){
			}

			template<typename	T>	Array<T>::~Array(){
			}

			template<typename	T>	inline	void	Array<T>::ensure(uint32	count){

				if(count<=_maxCount)
					return;
				uint32	s=count*sizeof(T)+sizeof(Storage<T>);
				uint32	normalizedSize;
				Storage<T>	*newStorage=new(s,normalizedSize)	Storage<T>();
				newStorage->setNormalizedSize(normalizedSize);
				if(_data!=NULL)
					memcpy(newStorage->data,_data->data,_data->count*sizeof(T));
				_data=newStorage;
				_maxCount=(normalizedSize-sizeof(Storage<T>))/sizeof(T);
			}

			template<typename	T>	inline	uint32	Array<T>::count()	const{

				return	_data->count;
			}

			template<typename	T>	inline	T	&Array<T>::operator	[](uint32	i){

				ensure(i+1);
				if(i>=_data->count)
					_data->count=i+1;
				return	_data->operator	[](i);
			}

			template<typename	T>	inline	uint8	*Array<T>::asBytes(uint32	i){

				return	(uint8	*)(_data+i);
			}

			template<typename	T>	inline	uint16	Array<T>::ptrCount()	const{

				return	1;
			}

			template<typename	T>	inline	__Payload	*Array<T>::getPtr(uint16	i)	const{

				return	_data;
			}

			template<typename	T>	inline	void	Array<T>::setPtr(uint16	i,__Payload	*p){

				_data=(Storage<T>	*)p;
			}*/
		}
	}
}