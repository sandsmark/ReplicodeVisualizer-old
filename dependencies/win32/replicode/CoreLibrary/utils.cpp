//	utils.cpp
//
//	Author: Eric Nivel, Thor List
//
//	BSD license:
//	Copyright (c) 2008, Eric Nivel, Thor List
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Eric Nivel or Thor List nor the
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

#include	"utils.h"

#if defined	WINDOWS
    #include	<intrin.h>
    #pragma	intrinsic (_InterlockedDecrement)
    #pragma	intrinsic (_InterlockedIncrement)
    #pragma	intrinsic (_InterlockedExchange)
    #pragma	intrinsic (_InterlockedExchange64)
    #pragma	intrinsic (_InterlockedCompareExchange)
    #pragma	intrinsic (_InterlockedCompareExchange64)
#elif defined LINUX
#elif defined APPLE
    #include <errno.h>
    #include <CoreFoundation/CoreFoundation.h>
	#include <sys/time.h>
#else
    #error Unknown platform!
#endif

#include <algorithm>
#include <cctype>


namespace	core{

	#if defined LINUX || defined APPLE
		bool CalcTimeout(struct timespec &timeout, uint32 ms) {

			struct timeval now;
			if (gettimeofday(&now, NULL) != 0)
				return false;

			timeout.tv_sec = now.tv_sec + ms / 1000;
			long us = now.tv_usec + ms % 1000;
			if (us >= 1000000) {
				timeout.tv_sec++;
				us -= 1000000;
			}
			timeout.tv_nsec = us * 1000; // usec -> nsec
			return true;
		}

		uint64 GetTime() {
			struct timeval tv; 
			if ( gettimeofday(&tv, NULL))
				return 0; 
			return (tv.tv_usec + tv.tv_sec * 1000000LL);
		}
	#endif

	void PrintBinary(void* p, uint32 size, bool asInt, const char* title) {
		if (title != NULL)
			printf("--- %s %u ---\n", title, (unsigned int)size);
		unsigned char c;
		for (uint32 n=0; n<size; n++) {
			c = *(((unsigned char*)p)+n);
			if (asInt)
				printf("[%u] ", (unsigned int)c);
			else
				printf("[%c] ", c);
			if ( (n > 0) && ((n+1)%10 == 0) )
				printf("\n");
		}
		printf("\n");
	}

	SharedLibrary	*SharedLibrary::New(const	char	*fileName){

		SharedLibrary	*sl=new	SharedLibrary();
		if(sl->load(fileName))
			return	sl;
		else{
		
			delete	sl;
			return	NULL;
		}
	}

	SharedLibrary::SharedLibrary():library(NULL){
	}

	SharedLibrary::~SharedLibrary(){
#if defined	WINDOWS
		if(library)
			FreeLibrary(library);
#elif defined LINUX || defined APPLE
		if(library)
			dlclose(library);
#endif
	}

	SharedLibrary	*SharedLibrary::load(const	char	*fileName){
#if defined	WINDOWS
		library=LoadLibrary(TEXT(fileName));
		if(!library){

			DWORD	error=GetLastError();
			std::cerr<<"> Error: unable to load shared library "<<fileName<<" :"<<error<<std::endl;
			return	NULL;
		}
#elif defined LINUX
		/*
		 * libraries on Linux are called 'lib<name>.so'
		 * if the passed in fileName does not have those
		 * components add them in.
		 */
		char *libraryName = (char *)calloc(1, strlen(fileName)+6+1);
		if (strstr(fileName, "lib") == NULL) {
			strcat(libraryName, "lib");
		}
		strcat(libraryName, fileName);
		if (strstr(fileName+(strlen(fileName)-3), ".so") == NULL) {
			strcat(libraryName, ".so");
		}
		library = dlopen(libraryName, RTLD_NOW | RTLD_GLOBAL);
		if (!library) {
			std::cout<<"> Error: unable to load shared library "<< fileName << " :" << dlerror() << std::endl;
			free(libraryName);
			return NULL;
		}
		free(libraryName);
#elif defined APPLE
		/*
		 * libraries on Apple are called '<name>.dylib'
		 * if the passed in fileName does not have those
		 * components add them in.
		 */
        // First, get bundle path
        char vBundlePath[1024];
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
        CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
        CFStringGetCString(cfStringRef, vBundlePath, 1024, kCFStringEncodingASCII);        
        CFRelease(mainBundleURL);
        CFRelease(cfStringRef);
        
		char *libraryName = (char *)calloc(1, strlen(fileName)+6+1+strlen(vBundlePath)+20);
		if ( fileName[0] != '/' )
        {   // If it's a relative path, put bundle path
			strcat( libraryName, vBundlePath );
			strcat( libraryName, "/Contents/MacOS/" );
		}        
		strcat(libraryName, fileName);
		if (strstr(fileName+(strlen(fileName)-3), ".dylib") == NULL) {
			strcat(libraryName, ".dylib");
		}
		library = dlopen(libraryName, RTLD_NOW | RTLD_GLOBAL);
		if (!library) {
			std::cout<<"> Error: unable to load shared library "<< fileName << " :" << dlerror() << std::endl;
			free(libraryName);
			return NULL;
		}
		free(libraryName);
#endif
		return	this;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	void	Thread::TerminateAndWait(Thread	**threads,uint32	threadCount){
		if(!threads)
			return;
		for(uint32	i=0;i<threadCount;i++) {
			threads[i]->terminate();
			Thread::Wait(threads[i]);
		}
	}

	void	Thread::TerminateAndWait(Thread	*_thread){
		if(!_thread)
			return;
		_thread->terminate();
		Thread::Wait(_thread);
	}

	void	Thread::Wait(Thread	**threads,uint32	threadCount){

		if(!threads)
			return;
#if defined	WINDOWS
		for(uint32	i=0;i<threadCount;i++)
			WaitForSingleObject(threads[i]->_thread,INFINITE);
#elif defined LINUX || defined APPLE
		for(uint32	i=0;i<threadCount;i++)
			pthread_join(threads[i]->_thread, NULL);
#endif
	}

	void	Thread::Wait(Thread	*_thread){

		if(!_thread)
			return;
#if defined	WINDOWS
		WaitForSingleObject(_thread->_thread,INFINITE);
#elif defined LINUX || defined APPLE
		pthread_join(_thread->_thread, NULL);
#endif
	}

	void	Thread::Sleep(int64	ms){
#if defined	WINDOWS
		::Sleep((uint32)ms);
#elif defined LINUX || defined APPLE
		// we are actually being passed millisecond, so multiply up
		usleep(ms*1000);
#endif
	}

	void	Thread::Sleep(){
#if defined	WINDOWS
		::Sleep(INFINITE);
#elif defined LINUX || defined APPLE
		while(true)
			sleep(1000);
#endif
	}

	Thread::Thread():is_meaningful(false){
		_thread = NULL;
	}

	Thread::~Thread(){
#if defined	WINDOWS
//		ExitThread(0);
		if(is_meaningful)
			CloseHandle(_thread);
#elif defined LINUX || defined APPLE
//		delete(_thread);
#endif
	}

	void	Thread::start(thread_function	f){
#if defined	WINDOWS
		_thread=CreateThread(NULL,65536,f,this,0,NULL);	//	64KB: minimum initial stack size
#elif defined LINUX || defined APPLE
		pthread_create(&_thread, NULL, f, this);
#endif
		is_meaningful=true;
	}

	void	Thread::suspend(){
#if defined	WINDOWS
		SuspendThread(_thread);
#elif defined LINUX || defined APPLE
		pthread_kill(_thread, SIGSTOP);
#endif
	}

	void	Thread::resume(){
#if defined	WINDOWS
		ResumeThread(_thread);
#elif defined LINUX || defined APPLE
		pthread_kill(_thread, SIGCONT);
#endif
	}

	void	Thread::terminate(){
#if defined	WINDOWS
		TerminateThread(_thread, 0);
#elif defined LINUX || defined APPLE
		pthread_cancel(_thread);
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	void	TimeProbe::set(){
		
		cpu_counts=getCounts();
	}

	int64	TimeProbe::getCounts(){
#if defined	WINDOWS
		LARGE_INTEGER	counter;
		QueryPerformanceCounter(&counter);
		return	counter.QuadPart;
#elif defined LINUX || defined APPLE
		static struct timeval tv;
		static struct timezone tz;
		gettimeofday(&tv, &tz);
		return (((int64)tv.tv_sec)*1000000) + (int64)tv.tv_usec;
#endif
	}

	void	TimeProbe::check(){

		cpu_counts=getCounts()-cpu_counts;
	}

	uint64	TimeProbe::us(){

		return	(uint64)(cpu_counts*Time::Period);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

#if defined	WINDOWS
	typedef LONG NTSTATUS;
	typedef NTSTATUS (__stdcall *NSTR)(ULONG, BOOLEAN, PULONG);
	#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
	bool	NtSetTimerResolution(IN	ULONG	RequestedResolution,IN	BOOLEAN	Set,OUT	PULONG	ActualResolution);
#elif defined LINUX || defined APPLE
	// TODO
#endif

	float64	Time::Period;
	
	int64	Time::InitTime;

	void	Time::Init(uint32	r){
#if defined	WINDOWS
	NTSTATUS	nts;
	HMODULE	NTDll=::LoadLibrary("NTDLL");
	ULONG	actualResolution=0;
	if(NTDll){

		NSTR	pNSTR=(NSTR)::GetProcAddress(NTDll,"NtSetTimerResolution");	//	undocumented win xp sys internals
		if(pNSTR)
			nts=(*pNSTR)(10*r,true,&actualResolution);	//	in 100 ns units
	}
	LARGE_INTEGER	f;
	QueryPerformanceFrequency(&f);
	Period=1000000.0/f.QuadPart;	//	in us
	struct	_timeb	local_time;
	_ftime(&local_time);
	InitTime=(int64)(local_time.time*1000+local_time.millitm)*1000;	//	in us
#elif defined LINUX || defined APPLE
	// we are actually setup a timer resolution of 1ms
	// we can simulate this by performing a gettimeofday call
	struct timeval tv;
	gettimeofday(&tv, NULL);
	InitTime = (((int64)tv.tv_sec) * 1000000) + (int64)tv.tv_usec;
	Period=1;	//	we measure all time in us anyway, so conversion is 1-to-1
#endif
	}

	uint64	Time::Get(){
#if defined	WINDOWS
		LARGE_INTEGER	counter;
		QueryPerformanceCounter(&counter);
		return	(uint64)(InitTime+counter.QuadPart*Period);
#elif defined LINUX || defined APPLE
		timeval perfCount;
		struct timezone tmzone;
		gettimeofday(&perfCount, &tmzone);
		int64 r = (((int64)perfCount.tv_sec) * 1000000) + (int64)perfCount.tv_usec;
		return	r;
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	uint8	Host::Name(char	*name){
#if defined	WINDOWS
		uint32	s=255;
		GetComputerName(name,&s);
		return	(uint8)s;
#elif defined LINUX || defined APPLE
		struct utsname utsname;
		uname(&utsname);
		strcpy(name, utsname.nodename);
		return strlen(name);
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

#if defined	WINDOWS
	const	uint32	Semaphore::Infinite=INFINITE;
#elif defined LINUX
	/*
	 * Normally this should be SEM_VALUE_MAX but apparently the <semaphore.h> header
	 * does not define it. The documents I have read indicate that on Linux it is
	 * always equal to INT_MAX - so use that instead.
	 */
	const	uint32	Semaphore::Infinite=INT_MAX;
#elif defined APPLE
	const	uint32	Semaphore::Infinite=SEM_VALUE_MAX;
#endif

	Semaphore::Semaphore(uint32	initialCount,uint32	maxCount){
#if defined	WINDOWS
		s=CreateSemaphore(NULL,initialCount,maxCount,NULL);
#elif defined LINUX
		bool res = sem_init(&s, 0, initialCount);
#elif defined APPLE
        static int vCount = 2000;
        char vName[256];
        sprintf( vName, "replicode%d", vCount++ );
		s = sem_open( vName, O_CREAT, 0x777, initialCount );
        int err = errno;
        int a = 0;
#endif
	}

	Semaphore::~Semaphore(){
#if defined	WINDOWS
		CloseHandle(s);
#elif defined LINUX
		sem_destroy(&s);
#elif defined APPLE
		sem_destroy(s);
#endif
	}

	bool	Semaphore::acquire(uint32	timeout){
#if defined	WINDOWS
		uint32	r=WaitForSingleObject(s,timeout);
		return	r==WAIT_TIMEOUT;
#elif defined LINUX
		struct timespec t;
		int r;
		
		CalcTimeout(t, timeout);
		r = sem_timedwait(&s, &t);
		return r == 0;
#elif defined APPLE
		int r;
		
		r = sem_wait(s);
		return r == 0;
#endif
	}

	void	Semaphore::release(uint32	count){
#if defined	WINDOWS
		ReleaseSemaphore(s,count,NULL);
#elif defined LINUX
		for (uint32 c = 0; c < count; c++)
			sem_post(&s);
#elif defined APPLE
		for (uint32 c = 0; c < count; c++)
			sem_post(s);
#endif
	}

	void	Semaphore::reset(){
#if defined	WINDOWS
		bool	r;
		do
			r=acquire(0);
		while(!r);
#elif defined LINUX || defined APPLE
		bool	r;
		do
			r=acquire(0);
		while(!r);
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

#if defined	WINDOWS
	const	uint32	Mutex::Infinite=INFINITE;
#elif defined LINUX || defined APPLE
	/*
	 * Normally this should be SEM_VALUE_MAX but apparently the <semaphore.h> header
	 * does not define it. The documents I have read indicate that on Linux it is
	 * always equal to INT_MAX - so use that instead.
	 */
	const	uint32	Mutex::Infinite=INT_MAX;
#endif

	Mutex::Mutex(){
#if defined	WINDOWS
		m=CreateMutex(NULL,false,NULL);
#elif defined LINUX || defined APPLE
		pthread_mutex_init(&m, NULL);
#endif
	}

	Mutex::~Mutex(){
#if defined	WINDOWS
		CloseHandle(m);
#elif defined LINUX || defined APPLE
		pthread_mutex_destroy(&m);
#endif
	}

	bool	Mutex::acquire(uint32	timeout){
#if defined	WINDOWS
		uint32	r=WaitForSingleObject(m,timeout);
		return	r==WAIT_TIMEOUT;
#elif defined LINUX || defined APPLE
		int64 start = Time::Get();
		int64 uTimeout = timeout*1000;

		while (pthread_mutex_trylock(&m) != 0) {
			Thread::Sleep(10);
			if (Time::Get() - start >= uTimeout)
				return false;
		}
		return true;

#endif
	}

	void	Mutex::release(){
#if defined	WINDOWS
		ReleaseMutex(m);
#elif defined LINUX || defined APPLE
		pthread_mutex_unlock(&m);
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	CriticalSection::CriticalSection(){
#if defined	WINDOWS
		InitializeCriticalSection(&cs);
#elif defined LINUX || defined APPLE
		pthread_mutex_init(&cs, NULL);
#endif
	}

	CriticalSection::~CriticalSection(){
#if defined	WINDOWS
		DeleteCriticalSection(&cs);
#elif defined LINUX || defined APPLE
		pthread_mutex_destroy(&cs);
#endif
	}

	void	CriticalSection::enter(){
#if defined	WINDOWS
		EnterCriticalSection(&cs);
#elif defined LINUX || defined APPLE
		pthread_mutex_lock(&cs);
#endif
	}

	void	CriticalSection::leave(){
#if defined	WINDOWS
		LeaveCriticalSection(&cs);
#elif defined LINUX || defined APPLE
		pthread_mutex_unlock(&cs);
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

#if defined	WINDOWS
	const	uint32	Timer::Infinite=INFINITE;
#elif defined LINUX || defined APPLE
	const	uint32	Timer::Infinite=INT_MAX;

	static void timer_signal_handler(int sig, siginfo_t *siginfo, void *context) { 
		SemaTex* sematex = (SemaTex*) siginfo->si_value.sival_ptr;
		if (sematex == NULL)
			return;
		pthread_mutex_lock(&sematex->mutex);
		pthread_cond_broadcast(&sematex->semaphore);
		pthread_mutex_unlock(&sematex->mutex);
	}
#elif defined APPLE
	const	uint32	Timer::Infinite=INT_MAX;
#endif

	Timer::Timer(){
	#if defined	WINDOWS
		t=CreateWaitableTimer(NULL,false,NULL);
		if (t == NULL) {
			printf("Error creating timer\n");
		}
	#elif defined LINUX
		pthread_cond_init(&sematex.semaphore, NULL);
		pthread_mutex_init(&sematex.mutex, NULL);

		struct sigaction sa;
		struct sigevent timer_event;

		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_SIGINFO;   /* Real-Time signal */
		sa.sa_sigaction = timer_signal_handler;
		sigaction(SIGRTMIN, &sa, NULL);

		timer_event.sigev_notify = SIGEV_SIGNAL;
		timer_event.sigev_signo = SIGRTMIN;
		timer_event.sigev_value.sival_ptr = (void *)&sematex;
		int ret = timer_create(CLOCK_REALTIME, &timer_event, &timer);
		if (ret != 0) {
			printf("Error creating timer: %d\n", ret);
		}
    #elif define APPLE
    #endif
	}

	Timer::~Timer(){
#if defined	WINDOWS
		CloseHandle(t);
#elif defined LINUX
		pthread_cond_destroy(&sematex.semaphore);
		pthread_mutex_destroy(&sematex.mutex);
		timer_delete(timer);
#elif define APPLE
#endif
	}

	void	Timer::start(uint64	deadline,uint32	period){
	#if defined	WINDOWS
		LARGE_INTEGER	_deadline;	//	in 100 ns intervals
		_deadline.QuadPart=-10LL*deadline;	//	negative means relative
		bool	r=SetWaitableTimer(t,&_deadline,(long)period,NULL,NULL,0);
		if (!r) {
			printf("Error arming timer\n");
		}
	#elif defined LINUX
		struct itimerspec newtv;
		sigset_t allsigs;

		uint64 t = deadline;
		uint64 p = period * 1000;
		newtv.it_interval.tv_sec = p / 1000000; 
		newtv.it_interval.tv_nsec = (p % 1000000)*1000; 
		newtv.it_value.tv_sec = t / 1000000; 
		newtv.it_value.tv_nsec = (t % 1000000)*1000; 
		
		pthread_mutex_lock(&sematex.mutex);

		int ret = timer_settime(timer, 0, &newtv, NULL);
		if (ret != 0) {
			printf("Error arming timer: %d\n", ret);
		}
		sigemptyset(&allsigs);

		pthread_mutex_unlock(&sematex.mutex);
    #elif define APPLE
	#endif
	}

	bool	Timer::wait(uint32	timeout){
	#if defined	WINDOWS
		uint32	r=WaitForSingleObject(t,timeout);
		return	r==WAIT_TIMEOUT;
	#elif defined LINUX
		bool res;
		struct timespec ttimeout;

		pthread_mutex_lock(&sematex.mutex);
		if (timeout == INT_MAX) {
			res = (pthread_cond_wait(&sematex.semaphore, &sematex.mutex) == 0);
		}
		else {
			CalcTimeout(ttimeout, timeout);
			res = (pthread_cond_timedwait(&sematex.semaphore, &sematex.mutex, &ttimeout) == 0);
		}
		pthread_mutex_unlock(&sematex.mutex);
		return res;
    #elif defined APPLE
        sleep( timeout );
	#endif
	}

	bool	Timer::wait(uint64	&us,uint32	timeout){

		TimeProbe	probe;
		probe.set();
		bool	r=wait(timeout);
		probe.check();
		us=probe.us();
		return	r;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	void	SignalHandler::Add(signal_handler	h){
#if defined	WINDOWS
		if(SetConsoleCtrlHandler(h,true)==0){

			int	e=GetLastError();
			std::cerr<<"Error: "<<e<<" failed to add signal handler"<<std::endl;
			return;
		}
#elif defined LINUX || defined APPLE
   signal(SIGABRT, SIG_IGN);
   signal(SIGPIPE, SIG_IGN);
   signal(SIGBUS, SIG_IGN);
//   signal(SIGHUP, h);
   signal(SIGTERM, h);
   signal(SIGINT, h);
   signal(SIGABRT, h);
//   signal(SIGFPE, h);
//   signal(SIGILL, h);
//   signal(SIGSEGV, h);
#endif
	}

	void	SignalHandler::Remove(signal_handler	h){
#if defined	WINDOWS
		SetConsoleCtrlHandler(h,false);
#elif defined LINUX || defined APPLE
   signal(SIGABRT, SIG_IGN);
   signal(SIGPIPE, SIG_IGN);
   signal(SIGBUS, SIG_IGN);
//   signal(SIGHUP, SIG_DFL);
   signal(SIGTERM, SIG_DFL);
   signal(SIGINT, SIG_DFL);
   signal(SIGABRT, SIG_DFL);
//   signal(SIGFPE, SIG_DFL);
//   signal(SIGILL, SIG_DFL);
//   signal(SIGSEGV, SIG_DFL);
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	int32	Atomic::Increment32(int32	volatile	*v){
#if defined	WINDOWS
		return	InterlockedIncrement(v);
#elif defined LINUX || defined APPLE
		__sync_add_and_fetch(v, 1);
		return	*v;
#endif
	};

	int32	Atomic::Decrement32(int32	volatile	*v){
#if defined	WINDOWS
		return	InterlockedDecrement(v);
#elif defined LINUX || defined APPLE
		__sync_add_and_fetch(v, -1);
		return	*v;
#endif
	};

	int32	Atomic::CompareAndSwap32(int32	volatile	*target,int32	v1,int32	v2){
#if defined	WINDOWS
		return	_InterlockedCompareExchange(target,v2,v1);
#elif defined LINUX || defined APPLE
		// note that v1 and v2 are swapped for Linux!!!
		return __sync_val_compare_and_swap(target, v1, v2);
#endif
	}

	int64	Atomic::CompareAndSwap64(int64	volatile	*target,int64	v1,int64	v2){
#if defined	WINDOWS
		return	_InterlockedCompareExchange64(target,v2,v1);
#elif defined LINUX || defined APPLE
		// note that v1 and v2 are swapped for Linux!!!
		return __sync_val_compare_and_swap(target, v1, v2);
#endif
	}

	word	Atomic::CompareAndSwap(word	volatile	*target,word	v1,word	v2){
#if defined	ARCH_32
		return	CompareAndSwap32(target,v1,v2);
#elif defined ARCH_64
		return	CompareAndSwap64(target,v1,v2);
#endif
	}

	int32	Atomic::Swap32(int32	volatile	*target,int32	v){
#if defined	WINDOWS
		return	_InterlockedExchange(target,v);
#elif defined LINUX || defined APPLE
		return __sync_fetch_and_sub(target,v);
#endif
	}

	int64	Atomic::Swap64(int64	volatile	*target,int64	v){
#if defined	WINDOWS
		return	CompareAndSwap64(target,v,v);
#elif defined LINUX || defined APPLE
		return __sync_fetch_and_sub(target,v);
#endif
	}

	word	Atomic::Swap(word	volatile	*target,word	v){
#if defined	ARCH_32
		return	Swap32(target,v);
#elif defined ARCH_64
		return	Swap64(target,v);
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	uint8	BSR(word	data){
#if defined	WINDOWS
#if defined	ARCH_32
		uint32	index;
		_BitScanReverse(&index,data);
		return	(uint8)index;
#elif defined	ARCH_64
		uint64	index;
		_BitScanReverse64(&index,data);
		return	(uint8)index;
#endif
#elif defined LINUX || defined APPLE
#if defined	ARCH_32
		return	(uint8)(31-__builtin_clz((uint32_t)data));
#elif defined	ARCH_64
		return	(uint8)(63-__builtin_clzll((uint64_t)data));
#endif
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	FastSemaphore::FastSemaphore(uint32	initialCount,uint32	maxCount):Semaphore(initialCount,maxCount),count(initialCount),maxCount(maxCount){
	}

	FastSemaphore::~FastSemaphore(){
	}

	void	FastSemaphore::acquire(){

		int32	c;
		while((c=Atomic::Decrement32(&count))>=maxCount);	//	release calls can bring count over maxCount: acquire has to exhaust these extras
		if(c<0)
			Semaphore::acquire();
	}

	void	FastSemaphore::release(){

		int32	c=Atomic::Increment32(&count);
		if(c<=maxCount)
			Semaphore::release();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
/*
	FastMutex::FastMutex(uint32	initialCount):Semaphore(initialCount,1),count(initialCount){
	}

	FastMutex::~FastMutex(){
	}

	void	FastMutex::acquire(){

		int32	former=Atomic::Swap32(&count,0);
		if(former==0)
			Semaphore::acquire();
	}

	void	FastMutex::release(){

		int32	former=Atomic::Swap32(&count,1);
		if(former==0)
			Semaphore::release();
	}
	*/
	////////////////////////////////////////////////////////////////////////////////////////////////

	bool	Error::PrintLastOSErrorMessage(const char* title) {
		int32 err = Error::GetLastOSErrorNumber();
		char buf[1024];
		if (!Error::GetOSErrorMessage(buf, 1024, err))
			printf("%s: [%d] (could not get error message)\n", title, (int)err);
		else
			printf("%s: [%d] %s\n", title, (int)err, buf);
		return true;
	}

	int32	Error::GetLastOSErrorNumber() {
		#ifdef WINDOWS
			int32 err = WSAGetLastError();
			WSASetLastError(0);
			return err;
        #elif defined APPLE
            return errno;
		#else
			return (int32) errno;
		#endif
	}

	bool	Error::GetOSErrorMessage(char* buffer, uint32 buflen, int32 err) {
		if (buffer == NULL)
			return false;
		if (buflen < 512) {
			strcpy(buffer, "String buffer not large enough");
			return false;
		}
		if (err < 0)
			err = Error::GetLastOSErrorNumber();

		#ifdef WINDOWS
			if (err == WSANOTINITIALISED) {
				strcpy(buffer, "Cannot initialize WinSock!");
			}
			else if (err == WSAENETDOWN) {
				strcpy(buffer, "The network subsystem or the associated service provider has failed");
			}
			else if (err == WSAEAFNOSUPPORT) {
				strcpy(buffer, "The specified address family is not supported");
			}
			else if (err == WSAEINPROGRESS) {
				strcpy(buffer, "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function");
			}
			else if (err == WSAEMFILE) {
				strcpy(buffer, "No more socket descriptors are available");
			}
			else if (err == WSAENOBUFS) {
				strcpy(buffer, "No buffer space is available. The socket cannot be created");
			}
			else if (err == WSAEPROTONOSUPPORT) {
				strcpy(buffer, "The specified protocol is not supported");
			}
			else if (err == WSAEPROTOTYPE) {
				strcpy(buffer, "The specified protocol is the wrong type for this socket");
			}
			else if (err == WSAESOCKTNOSUPPORT) {
				strcpy(buffer, "The specified socket type is not supported in this address family");
			}
			else if (err == WSAEADDRINUSE) {
				strcpy(buffer, "The socket's local address is already in use and the socket was not marked to allow address reuse with SO_REUSEADDR. This error usually occurs during execution of the bind function, but could be delayed until this function if the bind was to a partially wildcard address (involving ADDR_ANY) and if a specific address needs to be committed at the time of this function");
			}
			else if (err == WSAEINVAL) {
				strcpy(buffer, "The socket has not been bound with bind");
			}
			else if (err == WSAEISCONN) {
				strcpy(buffer, "The socket is already connected");
			}
			else if (err == WSAENOTSOCK) {
				strcpy(buffer, "The descriptor is not a socket");
			}
			else if (err == WSAEOPNOTSUPP) {
				strcpy(buffer, "The referenced socket is not of a type that supports the listen operation");
			}
			else if (err == WSAEADDRNOTAVAIL) {
				strcpy(buffer, "The specified address is not a valid address for this machine");
			}
			else if (err == WSAEFAULT) {
				strcpy(buffer, "The name or namelen parameter is not a valid part of the user address space, the namelen parameter is too small, the name parameter contains an incorrect address format for the associated address family, or the first two bytes of the memory block specified by name does not match the address family associated with the socket descriptor s");
			}
			else if (err == WSAEMFILE) {
				strcpy(buffer, "The queue is nonempty upon entry to accept and there are no descriptors available");
			}
			else if (err == WSAEWOULDBLOCK) {
				strcpy(buffer, "The socket is marked as nonblocking and no connections are present to be accepted");
			}
			else if (err == WSAETIMEDOUT) {
				strcpy(buffer, "Attempt to connect timed out without establishing a connection");
			}
			else if (err == WSAENETUNREACH) {
				strcpy(buffer, "The network cannot be reached from this host at this time");
			}
			else if (err == WSAEISCONN) {
				strcpy(buffer, "The socket is already connected (connection-oriented sockets only)");
			}
			else if (err == WSAECONNREFUSED) {
				strcpy(buffer, "The attempt to connect was forcefully rejected");
			}
			else if (err == WSAEAFNOSUPPORT) {
				strcpy(buffer, "Addresses in the specified family cannot be used with this socket");
			}
			else if (err == WSAEADDRNOTAVAIL) {
				strcpy(buffer, "The remote address is not a valid address (such as ADDR_ANY)");
			}
			else if (err == WSAEALREADY) {
				strcpy(buffer, "A nonblocking connect call is in progress on the specified socket");
			}
			else if (err == WSAECONNRESET) {
				strcpy(buffer, "Connection was reset");
			}
			else if (err == WSAECONNABORTED) {
				strcpy(buffer, "Software caused connection abort");
			}
			else {
				strcpy(buffer, "Socket error with no description");
			}

		#else
			strcpy(buffer, strerror(err));
		#endif

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	bool WaitForSocketReadability(socket s, int32 timeout) {

		int maxfd = 0;

		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 0;

		fd_set rdds;
		// create a list of sockets to check for activity
		FD_ZERO(&rdds);
		// specify mySocket
		FD_SET(s, &rdds);

		#ifdef WINDOWS
		#else
			maxfd = s + 1;
		#endif

		if (timeout > 0) {
			ldiv_t d = ldiv(timeout*1000, 1000000);
			tv.tv_sec = d.quot;
			tv.tv_usec = d.rem;
		}

		// Check for readability
		int ret = select(maxfd, &rdds, NULL, NULL, &tv);
		return(ret > 0);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	int32	String::StartsWith(const std::string &s, const std::string &str) {
		std::string::size_type pos = s.find_first_of(str);
		if (pos == 0)
			return 0;
		else
			return -1;
	}

	int32	String::EndsWith(const std::string &s, const std::string &str) {
		std::string::size_type pos = s.find_last_of(str);
		if (pos == s.size()-str.size())
			return pos;
		else
			return -1;
	}

	void	String::MakeUpper(std::string &str)
	{
		std::transform(str.begin(),str.end(),str.begin(),toupper);
	}

	void	String::MakeLower(std::string &str)
	{
		std::transform(str.begin(),str.end(),str.begin(),tolower);
	}

	void	String::Trim(std::string& str, const char* chars2remove)
	{
		TrimLeft(str, chars2remove);
		TrimRight(str, chars2remove);
	}

	void	String::TrimLeft(std::string& str, const char* chars2remove)
	{
		if (!str.empty())
		{
			std::string::size_type pos = str.find_first_not_of(chars2remove);

			if (pos != std::string::npos)
				str.erase(0,pos);
			else
				str.erase( str.begin() , str.end() ); // make empty
		}
	}

	void	String::TrimRight(std::string& str, const char* chars2remove)
	{
		if (!str.empty())
		{
			std::string::size_type pos = str.find_last_not_of(chars2remove);

			if (pos != std::string::npos)
				str.erase(pos+1);
			else
				str.erase( str.begin() , str.end() ); // make empty
		}
	}


	void	String::ReplaceLeading(std::string& str, const char* chars2replace, char c)
	{
		if (!str.empty())
		{
			std::string::size_type pos = str.find_first_not_of(chars2replace);

			if (pos != std::string::npos)
				str.replace(0,pos,pos,c);
			else
			{
				int n = str.size();
				str.replace(str.begin(),str.end()-1,n-1,c);
			}
		}
	}

	std::string	String::Int2String(int32 i) {
		char buffer[1024];
		sprintf(buffer,"%d",(int)i);
		return std::string(buffer);
	}
}