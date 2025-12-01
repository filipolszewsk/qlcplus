-------------------------------------
Translated Report (Full Report Below)
-------------------------------------

Process:               qlcplus [52024]
Path:                  /Users/USER/QLC+.app/Contents/MacOS/qlcplus
Identifier:            qlcplus
Version:               ???
Code Type:             ARM-64 (Native)
Parent Process:        launchd [1]
User ID:               501

Date/Time:             2025-12-01 20:05:56.5074 +0100
OS Version:            macOS 15.6.1 (24G90)
Report Version:        12
Anonymous UUID:        4944AA08-7DDB-B375-C327-A7109C14C831

Sleep/Wake UUID:       7C76A1CE-BFE6-4485-BDE6-2569193781E3

Time Awake Since Boot: 66000 seconds
Time Since Wake:       52 seconds

System Integrity Protection: enabled

Crashed Thread:        0  Dispatch queue: com.apple.main-thread

Exception Type:        EXC_BAD_ACCESS (SIGSEGV)
Exception Codes:       KERN_INVALID_ADDRESS at 0x0000000000000112
Exception Codes:       0x0000000000000001, 0x0000000000000112

Termination Reason:    Namespace SIGNAL, Code 11 Segmentation fault: 11
Terminating Process:   exc handler [52024]

VM Region Info: 0x112 is not in any region.  Bytes before following region: 4329209582
      REGION TYPE                    START - END         [ VSIZE] PRT/MAX SHRMOD  REGION DETAIL
      UNUSED SPACE AT START
--->  
      __TEXT                      1020a8000-1020b4000    [   48K] r-x/r-x SM=COW  /Users/USER/QLC+.app/Contents/MacOS/qlcplus

Thread 0 Crashed::  Dispatch queue: com.apple.main-thread
0   libqlcplusengine.dylib        	       0x102561158 QMap<int, AttributeOverride>::operator[](int const&) + 180
1   libqlcplusengine.dylib        	       0x102560a40 Function::requestAttributeOverride(int, double) + 124
2   libqlcplusui.dylib            	       0x1029032fc VCXYPad::slotPresetClicked(bool) + 2208
3   QtCore                        	       0x1056cadec void doActivate<false>(QObject*, int, void**) + 1480
4   QtWidgets                     	       0x10332c9c0 QAbstractButtonPrivate::emitClicked() + 112
5   QtWidgets                     	       0x10332c544 QAbstractButton::click() + 136
6   QtCore                        	       0x1056cadec void doActivate<false>(QObject*, int, void**) + 1480
7   libqlcplusengine.dylib        	       0x1024f501c InputOutputMap::inputValueChanged(unsigned int, unsigned int, unsigned char, QString const&) + 80
8   QtCore                        	       0x1056c2d50 QObject::event(QEvent*) + 324
9   QtWidgets                     	       0x10323075c QApplicationPrivate::notify_helper(QObject*, QEvent*) + 336
10  QtWidgets                     	       0x103231770 QApplication::notify(QObject*, QEvent*) + 468
11  QtCore                        	       0x10567b67c QCoreApplicationPrivate::sendPostedEvents(QObject*, int, QThreadData*) + 628
12  libqcocoa.dylib               	       0x105f868bc QCocoaEventDispatcherPrivate::processPostedEvents() + 380
13  libqcocoa.dylib               	       0x105f87b74 QCocoaEventDispatcherPrivate::postedEventsSourceCallback(void*) + 516
14  CoreFoundation                	       0x19f612b14 __CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__ + 28
15  CoreFoundation                	       0x19f612aa8 __CFRunLoopDoSource0 + 172
16  CoreFoundation                	       0x19f612814 __CFRunLoopDoSources0 + 232
17  CoreFoundation                	       0x19f611468 __CFRunLoopRun + 840
18  CoreFoundation                	       0x19f610a98 CFRunLoopRunSpecific + 572
19  HIToolbox                     	       0x1ab0b327c RunCurrentEventLoopInMode + 324
20  HIToolbox                     	       0x1ab0b64e8 ReceiveNextEventCommon + 676
21  HIToolbox                     	       0x1ab241484 _BlockUntilNextEventMatchingListInModeWithFilter + 76
22  AppKit                        	       0x1a3535a34 _DPSNextEvent + 684
23  AppKit                        	       0x1a3ed4940 -[NSApplication(NSEventRouting) _nextEventMatchingEventMask:untilDate:inMode:dequeue:] + 688
24  AppKit                        	       0x1a3528be4 -[NSApplication run] + 480
25  libqcocoa.dylib               	       0x105f853a0 QCocoaEventDispatcher::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) + 2152
26  QtCore                        	       0x10568397c QEventLoop::exec(QFlags<QEventLoop::ProcessEventsFlag>) + 588
27  QtCore                        	       0x10567aaa0 QCoreApplication::exec() + 228
28  qlcplus                       	       0x1020b1954 main + 1156
29  dyld                          	       0x19f186b98 start + 6076

Thread 1:: com.apple.NSEventThread
0   libsystem_kernel.dylib        	       0x19f4e5c34 mach_msg2_trap + 8
1   libsystem_kernel.dylib        	       0x19f4f83a0 mach_msg2_internal + 76
2   libsystem_kernel.dylib        	       0x19f4ee764 mach_msg_overwrite + 484
3   libsystem_kernel.dylib        	       0x19f4e5fa8 mach_msg + 24
4   CoreFoundation                	       0x19f612cbc __CFRunLoopServiceMachPort + 160
5   CoreFoundation                	       0x19f6115d8 __CFRunLoopRun + 1208
6   CoreFoundation                	       0x19f610a98 CFRunLoopRunSpecific + 572
7   AppKit                        	       0x1a365978c _NSEventThread + 140
8   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
9   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 2:: HPMPrivate
0   libsystem_kernel.dylib        	       0x19f4e5c34 mach_msg2_trap + 8
1   libsystem_kernel.dylib        	       0x19f4f83a0 mach_msg2_internal + 76
2   libsystem_kernel.dylib        	       0x19f4ee764 mach_msg_overwrite + 484
3   libsystem_kernel.dylib        	       0x19f4e5fa8 mach_msg + 24
4   CoreFoundation                	       0x19f612cbc __CFRunLoopServiceMachPort + 160
5   CoreFoundation                	       0x19f6115d8 __CFRunLoopRun + 1208
6   CoreFoundation                	       0x19f610a98 CFRunLoopRunSpecific + 572
7   CoreFoundation                	       0x19f68a554 CFRunLoopRun + 64
8   libqlcplusengine.dylib        	       0x10260e1f0 HPMPrivate::run() + 264
9   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
10  libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
11  libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 3:: com.apple.CFSocket.private
0   libsystem_kernel.dylib        	       0x19f4f0c2c __select + 8
1   CoreFoundation                	       0x19f638b30 __CFSocketManager + 704
2   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
3   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 4:
0   libsystem_kernel.dylib        	       0x19f4e5c34 mach_msg2_trap + 8
1   libsystem_kernel.dylib        	       0x19f4f83a0 mach_msg2_internal + 76
2   libsystem_kernel.dylib        	       0x19f4ee764 mach_msg_overwrite + 484
3   libsystem_kernel.dylib        	       0x19f4e5fa8 mach_msg + 24
4   CoreMIDI                      	       0x1bbbb6eac XServerMachPort::ReceiveMessage(int&, void*, int&) + 104
5   CoreMIDI                      	       0x1bbbd8740 MIDIProcess::MIDIInPortThread::Run() + 148
6   CoreMIDI                      	       0x1bbbcd76c CADeprecated::XThread::RunHelper(void*) + 48
7   CoreMIDI                      	       0x1bbbd7e44 CADeprecated::CAPThread::Entry(CADeprecated::CAPThread*) + 96
8   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
9   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 5:: org.libusb.device-hotplug
0   libsystem_kernel.dylib        	       0x19f4e5c34 mach_msg2_trap + 8
1   libsystem_kernel.dylib        	       0x19f4f83a0 mach_msg2_internal + 76
2   libsystem_kernel.dylib        	       0x19f4ee764 mach_msg_overwrite + 484
3   libsystem_kernel.dylib        	       0x19f4e5fa8 mach_msg + 24
4   CoreFoundation                	       0x19f612cbc __CFRunLoopServiceMachPort + 160
5   CoreFoundation                	       0x19f6115d8 __CFRunLoopRun + 1208
6   CoreFoundation                	       0x19f610a98 CFRunLoopRunSpecific + 572
7   CoreFoundation                	       0x19f68a554 CFRunLoopRun + 64
8   libusb-1.0.0.dylib            	       0x10de1c460 darwin_event_thread_main + 408
9   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
10  libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 6:: caulk.messenger.shared:17
0   libsystem_kernel.dylib        	       0x19f4e5bb0 semaphore_wait_trap + 8
1   caulk                         	       0x1aab9acc8 caulk::semaphore::timed_wait(double) + 224
2   caulk                         	       0x1aab9ab70 caulk::concurrent::details::worker_thread::run() + 32
3   caulk                         	       0x1aab9a844 void* caulk::thread_proxy<std::__1::tuple<caulk::thread::attributes, void (caulk::concurrent::details::worker_thread::*)(), std::__1::tuple<caulk::concurrent::details::worker_thread*>>>(void*) + 96
4   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
5   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 7:: caulk.messenger.shared:high
0   libsystem_kernel.dylib        	       0x19f4e5bb0 semaphore_wait_trap + 8
1   caulk                         	       0x1aab9acc8 caulk::semaphore::timed_wait(double) + 224
2   caulk                         	       0x1aab9ab70 caulk::concurrent::details::worker_thread::run() + 32
3   caulk                         	       0x1aab9a844 void* caulk::thread_proxy<std::__1::tuple<caulk::thread::attributes, void (caulk::concurrent::details::worker_thread::*)(), std::__1::tuple<caulk::concurrent::details::worker_thread*>>>(void*) + 96
4   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
5   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 8:: QThread
0   libsystem_kernel.dylib        	       0x19f4e91c8 __semwait_signal + 8
1   libsystem_c.dylib             	       0x19f3c56f4 nanosleep + 220
2   libqlcplusengine.dylib        	       0x1025ec300 MasterTimerPrivate::run() + 620
3   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
4   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
5   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 9:: QThread
0   libsystem_kernel.dylib        	       0x19f4ee498 poll + 8
1   QtCore                        	       0x105801268 qt_safe_poll(pollfd*, unsigned int, QDeadlineTimer) + 76
2   QtCore                        	       0x1058051cc QEventDispatcherUNIX::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) + 880
3   QtCore                        	       0x10568397c QEventLoop::exec(QFlags<QEventLoop::ProcessEventsFlag>) + 588
4   QtCore                        	       0x105770790 QThread::exec() + 336
5   libqlcplusengine.dylib        	       0x1025f2144 JSThread::run() + 56
6   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
7   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
8   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 10:: Universe
0   libsystem_kernel.dylib        	       0x19f4f3a54 __ulock_wait2 + 8
1   QtCore                        	       0x10580a7c4 bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer) + 260
2   QtCore                        	       0x10580a504 QSemaphore::tryAcquire(int, QDeadlineTimer) + 208
3   libqlcplusengine.dylib        	       0x1025df6b0 Universe::run() + 284
4   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
5   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
6   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 11:: Universe
0   libsystem_kernel.dylib        	       0x19f4f3a54 __ulock_wait2 + 8
1   QtCore                        	       0x10580a7c4 bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer) + 260
2   QtCore                        	       0x10580a504 QSemaphore::tryAcquire(int, QDeadlineTimer) + 208
3   libqlcplusengine.dylib        	       0x1025df6b0 Universe::run() + 284
4   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
5   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
6   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 12:: Universe
0   libsystem_kernel.dylib        	       0x19f4f3a54 __ulock_wait2 + 8
1   QtCore                        	       0x10580a7c4 bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer) + 260
2   QtCore                        	       0x10580a504 QSemaphore::tryAcquire(int, QDeadlineTimer) + 208
3   libqlcplusengine.dylib        	       0x1025df6b0 Universe::run() + 284
4   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
5   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
6   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 13:: Universe
0   libsystem_kernel.dylib        	       0x19f4f3a54 __ulock_wait2 + 8
1   QtCore                        	       0x10580a7c4 bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer) + 260
2   QtCore                        	       0x10580a504 QSemaphore::tryAcquire(int, QDeadlineTimer) + 208
3   libqlcplusengine.dylib        	       0x1025df6b0 Universe::run() + 284
4   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
5   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
6   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 14:: Universe
0   libsystem_kernel.dylib        	       0x19f4f3a54 __ulock_wait2 + 8
1   QtCore                        	       0x10580a7c4 bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer) + 260
2   QtCore                        	       0x10580a504 QSemaphore::tryAcquire(int, QDeadlineTimer) + 208
3   libqlcplusengine.dylib        	       0x1025df6b0 Universe::run() + 284
4   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
5   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
6   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 15:: Universe
0   libsystem_kernel.dylib        	       0x19f4f3a54 __ulock_wait2 + 8
1   QtCore                        	       0x10580a7c4 bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer) + 260
2   QtCore                        	       0x10580a504 QSemaphore::tryAcquire(int, QDeadlineTimer) + 208
3   libqlcplusengine.dylib        	       0x1025df6b0 Universe::run() + 284
4   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
5   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
6   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 16:: Universe
0   libsystem_kernel.dylib        	       0x19f4f3a54 __ulock_wait2 + 8
1   QtCore                        	       0x10580a7c4 bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer) + 260
2   QtCore                        	       0x10580a504 QSemaphore::tryAcquire(int, QDeadlineTimer) + 208
3   libqlcplusengine.dylib        	       0x1025df6b0 Universe::run() + 284
4   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
5   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
6   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 17:: Universe
0   libsystem_kernel.dylib        	       0x19f4f3a54 __ulock_wait2 + 8
1   QtCore                        	       0x10580a7c4 bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer) + 260
2   QtCore                        	       0x10580a504 QSemaphore::tryAcquire(int, QDeadlineTimer) + 208
3   libqlcplusengine.dylib        	       0x1025df6b0 Universe::run() + 284
4   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
5   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
6   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 18:: Universe
0   libsystem_kernel.dylib        	       0x19f4f3a54 __ulock_wait2 + 8
1   QtCore                        	       0x10580a7c4 bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer) + 260
2   QtCore                        	       0x10580a504 QSemaphore::tryAcquire(int, QDeadlineTimer) + 208
3   libqlcplusengine.dylib        	       0x1025df6b0 Universe::run() + 284
4   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
5   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
6   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 19:
0   libsystem_pthread.dylib       	       0x19f522b6c start_wqthread + 0

Thread 20:
0   libsystem_pthread.dylib       	       0x19f522b6c start_wqthread + 0

Thread 21:: Thread (pooled)
0   libsystem_kernel.dylib        	       0x19f4e93cc __psynch_cvwait + 8
1   libsystem_pthread.dylib       	       0x19f5280e0 _pthread_cond_wait + 984
2   QtCore                        	       0x1058105b4 QWaitConditionPrivate::wait(QDeadlineTimer) + 236
3   QtCore                        	       0x105810460 QWaitCondition::wait(QMutex*, QDeadlineTimer) + 108
4   QtCore                        	       0x10580acc8 QThreadPoolThread::run() + 932
5   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
6   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
7   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 22:: Thread (pooled)
0   libsystem_kernel.dylib        	       0x19f4e93cc __psynch_cvwait + 8
1   libsystem_pthread.dylib       	       0x19f5280e0 _pthread_cond_wait + 984
2   QtCore                        	       0x1058105b4 QWaitConditionPrivate::wait(QDeadlineTimer) + 236
3   QtCore                        	       0x105810460 QWaitCondition::wait(QMutex*, QDeadlineTimer) + 108
4   QtCore                        	       0x10580acc8 QThreadPoolThread::run() + 932
5   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
6   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
7   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 23:: Thread (pooled)
0   libsystem_kernel.dylib        	       0x19f4e93cc __psynch_cvwait + 8
1   libsystem_pthread.dylib       	       0x19f5280e0 _pthread_cond_wait + 984
2   QtCore                        	       0x1058105b4 QWaitConditionPrivate::wait(QDeadlineTimer) + 236
3   QtCore                        	       0x105810460 QWaitCondition::wait(QMutex*, QDeadlineTimer) + 108
4   QtCore                        	       0x10580acc8 QThreadPoolThread::run() + 932
5   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
6   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
7   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 24:: Thread (pooled)
0   libsystem_kernel.dylib        	       0x19f4e93cc __psynch_cvwait + 8
1   libsystem_pthread.dylib       	       0x19f5280e0 _pthread_cond_wait + 984
2   QtCore                        	       0x1058105b4 QWaitConditionPrivate::wait(QDeadlineTimer) + 236
3   QtCore                        	       0x105810460 QWaitCondition::wait(QMutex*, QDeadlineTimer) + 108
4   QtCore                        	       0x10580acc8 QThreadPoolThread::run() + 932
5   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
6   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
7   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 25:: Thread (pooled)
0   libsystem_kernel.dylib        	       0x19f4e93cc __psynch_cvwait + 8
1   libsystem_pthread.dylib       	       0x19f5280e0 _pthread_cond_wait + 984
2   QtCore                        	       0x1058105b4 QWaitConditionPrivate::wait(QDeadlineTimer) + 236
3   QtCore                        	       0x105810460 QWaitCondition::wait(QMutex*, QDeadlineTimer) + 108
4   QtCore                        	       0x10580acc8 QThreadPoolThread::run() + 932
5   QtCore                        	       0x105802d00 QThreadPrivate::start(void*) + 364
6   libsystem_pthread.dylib       	       0x19f527c0c _pthread_start + 136
7   libsystem_pthread.dylib       	       0x19f522b80 thread_start + 8

Thread 26:
0   libsystem_pthread.dylib       	       0x19f522b6c start_wqthread + 0

Thread 27:
0   libsystem_pthread.dylib       	       0x19f522b6c start_wqthread + 0

Thread 28:
0   libsystem_pthread.dylib       	       0x19f522b6c start_wqthread + 0


Thread 0 crashed with ARM Thread State (64-bit):
    x0: 0x00000001347fe2e0   x1: 0x000000016dd55284   x2: 0x000000000000004c   x3: 0x0000000000000000
    x4: 0x000000016dd55298   x5: 0x000000009443e887   x6: 0x00006000006f6180   x7: 0x0000000005f5d548
    x8: 0x00000000000000f2   x9: 0x0000000000000000  x10: 0x000060000002db88  x11: 0x000060000002db88
   x12: 0x00006000016adef8  x13: 0x0000000000000033  x14: 0x00000001596f8400  x15: 0x00000000800032f9
   x16: 0x00000001025609c4  x17: 0x0000000000000087  x18: 0x0000000000000000  x19: 0x0000600001c911d0
   x20: 0x0000600001c911c0  x21: 0x0000000000000084  x22: 0x0000600001c911d0  x23: 0x00000000000000f2
   x24: 0x000000010e10cfe8  x25: 0x000000010e10cfe0  x26: 0x0000000000000000  x27: 0x0000000000000090
   x28: 0x0000000000000001   fp: 0x000000016dd55240   lr: 0x00000001025610f8
    sp: 0x000000016dd551e0   pc: 0x0000000102561158 cpsr: 0x80001000
   far: 0x0000000000000112  esr: 0x92000006 (Data Abort) byte read Translation fault

Binary Images:
       0x1020a8000 -        0x1020b3fff qlcplus (*) <ff96842b-07be-35bd-a984-202ce8519030> /Users/USER/QLC+.app/Contents/MacOS/qlcplus
       0x102240000 -        0x1022a7fff libqlcpluswebaccess.dylib (*) <8316d4e8-a6d4-32fd-8b57-fed552ae629d> /Users/USER/QLC+.app/Contents/Frameworks/libqlcpluswebaccess.dylib
       0x10270c000 -        0x1029e7fff libqlcplusui.dylib (*) <4b88b4df-026d-32df-924c-28594207686e> /Users/USER/QLC+.app/Contents/Frameworks/libqlcplusui.dylib
       0x1024e8000 -        0x102637fff libqlcplusengine.dylib (*) <0a2f005b-2324-3650-8fc2-46ecc2a1b3af> /Users/USER/QLC+.app/Contents/Frameworks/libqlcplusengine.dylib
       0x1022d8000 -        0x10235ffff libfftw3.3.dylib (*) <407baed4-d9b0-3020-ac2c-c045b21ce470> /opt/homebrew/*/libfftw3.3.dylib
       0x1020e4000 -        0x1020ebfff org.qt-project.QtMultimediaWidgets (6.9) <bc2a69c6-2538-3bad-832c-f1fa54330dfc> /opt/homebrew/*/QtMultimediaWidgets.framework/Versions/A/QtMultimediaWidgets
       0x103224000 -        0x10368ffff org.qt-project.QtWidgets (6.9) <bda58d3c-fd8f-3a83-995f-e3fb4bbe1caf> /opt/homebrew/*/QtWidgets.framework/Versions/A/QtWidgets
       0x103908000 -        0x103cbffff org.qt-project.QtQml (6.9) <62666d6c-4c42-39ca-a1f3-89e7072ad5a5> /opt/homebrew/*/QtQml.framework/Versions/A/QtQml
       0x102cc8000 -        0x102d93fff org.qt-project.QtMultimedia (6.9) <91292b5a-4a41-3d49-a68f-e0622c017264> /opt/homebrew/*/QtMultimedia.framework/Versions/A/QtMultimedia
       0x104740000 -        0x104cbbfff org.qt-project.QtGui (6.9) <ec61ea9a-877a-3f04-b917-1f03807e1beb> /opt/homebrew/*/QtGui.framework/Versions/A/QtGui
       0x102148000 -        0x102167fff org.qt-project.QtWebSockets (6.9) <d7d2f3ec-e25f-348e-bc8a-a4009461dbd6> /opt/homebrew/*/QtWebSockets.framework/Versions/A/QtWebSockets
       0x103f2c000 -        0x10405ffff org.qt-project.QtNetwork (6.9) <2b65f08f-d022-3c82-8916-db1c522e3cc9> /opt/homebrew/*/QtNetwork.framework/Versions/A/QtNetwork
       0x1055e0000 -        0x105a47fff org.qt-project.QtCore (6.9) <2790f9c7-9b75-3f4e-96b7-ed028bd42288> /opt/homebrew/*/QtCore.framework/Versions/A/QtCore
       0x102100000 -        0x10210bfff libbrotlidec.1.1.0.dylib (*) <cc77e640-3de5-3d9c-9565-c60ed8119746> /opt/homebrew/*/libbrotlidec.1.1.0.dylib
       0x102428000 -        0x1024b3fff libzstd.1.5.7.dylib (*) <afd03688-5fac-3b3d-96aa-9d88c034ff23> /opt/homebrew/*/libzstd.1.5.7.dylib
       0x102e4c000 -        0x102edffff libssl.3.dylib (*) <810d99f1-2504-3db7-9357-4bce9d47fc76> /opt/homebrew/*/libssl.3.dylib
       0x104f5c000 -        0x10529bfff libcrypto.3.dylib (*) <efba4c7a-a8ab-3429-8076-3bf9f22bd40b> /opt/homebrew/*/libcrypto.3.dylib
       0x105c68000 -        0x105e1bfff libicui18n.77.1.dylib (*) <6c0f5599-2c90-33c5-8a77-33c654b11c06> /opt/homebrew/*/libicui18n.77.1.dylib
       0x104140000 -        0x104277fff libicuuc.77.1.dylib (*) <c48fd9eb-06bb-3a7b-b4c7-a6e7da9f8b92> /opt/homebrew/*/libicuuc.77.1.dylib
       0x107e20000 -        0x109c8bfff libicudata.77.1.dylib (*) <e996519b-b7ad-3d2c-94e9-1ca42fc9cfb0> /opt/homebrew/*/libicudata.77.1.dylib
       0x102f2c000 -        0x103027fff libglib-2.0.0.dylib (*) <a7cd8b10-d5ee-3abe-b185-81f2be4a1765> /opt/homebrew/*/libglib-2.0.0.dylib
       0x102118000 -        0x102123fff libdouble-conversion.3.3.0.dylib (*) <99e1194f-a3c6-30df-a9a2-5183f8ea3775> /opt/homebrew/*/libdouble-conversion.3.3.0.dylib
       0x102190000 -        0x102197fff libb2.1.dylib (*) <82f85bf6-93d0-3be3-9cc8-23cbf361e60f> /opt/homebrew/*/libb2.1.dylib
       0x102b44000 -        0x102baffff libpcre2-16.0.dylib (*) <f5623285-c810-3d39-8f96-8fc66d5b0601> /opt/homebrew/*/libpcre2-16.0.dylib
       0x102134000 -        0x102137fff libgthread-2.0.0.dylib (*) <058d0a42-f905-36cc-b767-c47265856de1> /opt/homebrew/*/libgthread-2.0.0.dylib
       0x1023c0000 -        0x1023e7fff libintl.8.dylib (*) <9414c162-5ccc-3b6d-8a0d-c7c743f7fec1> /opt/homebrew/*/libintl.8.dylib
       0x10307c000 -        0x1030effff libpcre2-8.0.dylib (*) <cb5b10c2-f523-342e-ac8d-aeb83b1612c6> /opt/homebrew/*/libpcre2-8.0.dylib
       0x102384000 -        0x1023a3fff libbrotlicommon.1.1.0.dylib (*) <cbb61532-d27c-331e-996e-843ed45f10e9> /opt/homebrew/*/libbrotlicommon.1.1.0.dylib
       0x103108000 -        0x103187fff org.qt-project.QtDBus (6.9) <8b57c0ac-f526-303c-994c-b50c656b1db6> /opt/homebrew/*/QtDBus.framework/Versions/A/QtDBus
       0x102bc8000 -        0x102bebfff libpng16.16.dylib (*) <1d486868-3c8c-321e-9727-3a5ba5cd220e> /opt/homebrew/*/libpng16.16.dylib
       0x104448000 -        0x10450bfff libharfbuzz.0.dylib (*) <00ed5fa9-de3f-3de0-b432-857eb401c764> /opt/homebrew/*/libharfbuzz.0.dylib
       0x1021a8000 -        0x1021b3fff libmd4c.0.5.2.dylib (*) <bbb78130-97b1-3019-9bb2-db21119d3562> /opt/homebrew/*/libmd4c.0.5.2.dylib
       0x104308000 -        0x104383fff libfreetype.6.dylib (*) <5642e153-03c7-3193-9276-871551a3a55e> /opt/homebrew/*/libfreetype.6.dylib
       0x102c4c000 -        0x102c7bfff libdbus-1.3.dylib (*) <fb8085dc-9746-335c-90d0-429dfe91dd60> /opt/homebrew/*/libdbus-1.3.dylib
       0x102bfc000 -        0x102c0ffff libgraphite2.3.2.1.dylib (*) <dadd250f-e2ba-36f3-b960-ea391395d1e7> /opt/homebrew/*/libgraphite2.3.2.1.dylib
       0x1043e8000 -        0x104407fff com.apple.security.csparser (3.0) <3a905673-ada9-3c57-992e-b83f555baa61> /System/Library/Frameworks/Security.framework/Versions/A/PlugIns/csparser.bundle/Contents/MacOS/csparser
       0x105f70000 -        0x106017fff libqcocoa.dylib (*) <5fb652bc-d296-3802-8f4d-a9677091961d> /opt/homebrew/*/libqcocoa.dylib
       0x10689c000 -        0x1068a7fff libobjc-trampolines.dylib (*) <a3faee04-0f8b-3428-9497-560c97eca6fb> /usr/lib/libobjc-trampolines.dylib
       0x10c86c000 -        0x10c88ffff libqmacstyle.dylib (*) <1559963e-26f9-3da8-9bba-9e3a97456f32> /opt/homebrew/*/libqmacstyle.dylib
       0x10e9c0000 -        0x10f057fff com.apple.AGXMetalG13X (329.2) <6b497f3b-6583-398c-9d05-5f30a1c1bae5> /System/Library/Extensions/AGXMetalG13X.bundle/Contents/MacOS/AGXMetalG13X
       0x10dd20000 -        0x10dd4bfff libartnet.dylib (*) <0f8a39b7-2edf-31be-b4a3-e2e4fdf19af4> /Users/USER/QLC+.app/Contents/PlugIns/libartnet.dylib
       0x10de3c000 -        0x10de7bfff libdmxusb.dylib (*) <127413c3-c54e-384d-854f-50c6e5fcf060> /Users/USER/QLC+.app/Contents/PlugIns/libdmxusb.dylib
       0x10ddcc000 -        0x10ddd7fff libftdi1.2.5.0.dylib (*) <3abe7ef1-b602-390b-a03f-6ddf5d65150c> /opt/homebrew/*/libftdi1.2.5.0.dylib
       0x10de10000 -        0x10de27fff libusb-1.0.0.dylib (*) <b0d386d3-de68-3ac0-97aa-3ac07386ede9> /opt/homebrew/*/libusb-1.0.0.dylib
       0x10dee0000 -        0x10def3fff org.qt-project.QtSerialPort (6.9) <093c8ddf-30db-3586-8f3e-3e2ef9737b21> /opt/homebrew/*/QtSerialPort.framework/Versions/A/QtSerialPort
       0x10df54000 -        0x10df73fff libe131.dylib (*) <bea0bccb-13ca-307f-8ea3-4b9de8d78f06> /Users/USER/QLC+.app/Contents/PlugIns/libe131.dylib
       0x10d94c000 -        0x10d95ffff libenttecwing.dylib (*) <19222aa1-fe97-39b5-962e-0f27812a5ea0> /Users/USER/QLC+.app/Contents/PlugIns/libenttecwing.dylib
       0x10df14000 -        0x10df2bfff libhidplugin.dylib (*) <39ff51e6-b47a-30dc-8ba8-d1db569837fa> /Users/USER/QLC+.app/Contents/PlugIns/libhidplugin.dylib
       0x10dde4000 -        0x10ddfbfff com.apple.iokit.IOHIDLib (2.0.0) <fdc90f28-4817-367a-b9e1-c0b89d530837> /System/Library/Extensions/IOHIDFamily.kext/Contents/PlugIns/IOHIDLib.plugin/Contents/MacOS/IOHIDLib
       0x10dbf4000 -        0x10dbfffff libloopback.dylib (*) <5ec40bd1-182b-3928-83c7-0b28eb555e9d> /Users/USER/QLC+.app/Contents/PlugIns/libloopback.dylib
       0x10e09c000 -        0x10e0bbfff libmidiplugin.dylib (*) <1668f041-c000-327f-90a2-0125e86d71a8> /Users/USER/QLC+.app/Contents/PlugIns/libmidiplugin.dylib
       0x10deac000 -        0x10debbfff libos2l.dylib (*) <07cea6d4-5111-37b2-b3b1-e83ae133c2c5> /Users/USER/QLC+.app/Contents/PlugIns/libos2l.dylib
       0x10e200000 -        0x10e21ffff libosc.dylib (*) <3d58eb73-76b9-3905-ae1d-551648aed33c> /Users/USER/QLC+.app/Contents/PlugIns/libosc.dylib
       0x10dfec000 -        0x10dffffff libpeperoni.dylib (*) <1af322fd-d2e4-3f77-8642-1d682595baab> /Users/USER/QLC+.app/Contents/PlugIns/libpeperoni.dylib
       0x10e018000 -        0x10e027fff libudmx.dylib (*) <0de40b91-da1c-3ac4-b3d6-d33d4d74b877> /Users/USER/QLC+.app/Contents/PlugIns/libudmx.dylib
       0x10e0dc000 -        0x10e0e7fff libvelleman.dylib (*) <7dede5e5-b1d5-3955-a231-172c68867ac9> /Users/USER/QLC+.app/Contents/PlugIns/libvelleman.dylib
       0x19f596000 -        0x19fad4fff com.apple.CoreFoundation (6.9) <8d45baee-6cc0-3b89-93fd-ea1c8e15c6d7> /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation
       0x1aaff0000 -        0x1ab2f6fdf com.apple.HIToolbox (2.1.1) <1a037942-11e0-3fc8-aad2-20b11e7ae1a4> /System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/HIToolbox.framework/Versions/A/HIToolbox
       0x1a34fb000 -        0x1a498be3f com.apple.AppKit (6.9) <860c164c-d04c-30ff-8c6f-e672b74caf11> /System/Library/Frameworks/AppKit.framework/Versions/C/AppKit
       0x19f180000 -        0x19f21b577 dyld (*) <3247e185-ced2-36ff-9e29-47a77c23e004> /usr/lib/dyld
               0x0 - 0xffffffffffffffff ??? (*) <00000000-0000-0000-0000-000000000000> ???
       0x19f4e5000 -        0x19f520653 libsystem_kernel.dylib (*) <6e4a96ad-04b8-3e8a-b91d-087e62306246> /usr/lib/system/libsystem_kernel.dylib
       0x19f521000 -        0x19f52da47 libsystem_pthread.dylib (*) <d6494ba9-171e-39fc-b1aa-28ecf87975d1> /usr/lib/system/libsystem_pthread.dylib
       0x1fff76000 -        0x2000ab1ff com.apple.audio.AVFAudio (1.0) <bf6d4128-7675-3fa9-9519-cf4dae7d7bab> /System/Library/Frameworks/AVFAudio.framework/Versions/A/AVFAudio
       0x1bbb78000 -        0x1bbc32d1f com.apple.audio.midi.CoreMIDI (2.0) <504d9a4a-f0a7-348f-a7bc-13fd26b48d99> /System/Library/Frameworks/CoreMIDI.framework/Versions/A/CoreMIDI
       0x1aab99000 -        0x1aabc0ddf com.apple.audio.caulk (1.0) <42085f32-42e2-3f11-b0b4-0343137b5f72> /System/Library/PrivateFrameworks/caulk.framework/Versions/A/caulk
       0x19f3b8000 -        0x19f439243 libsystem_c.dylib (*) <dfea8794-80ce-37c3-8f6a-108aa1d0b1b0> /usr/lib/system/libsystem_c.dylib

External Modification Summary:
  Calls made by other processes targeting this process:
    task_for_pid: 0
    thread_create: 0
    thread_set_state: 0
  Calls made by this process:
    task_for_pid: 0
    thread_create: 0
    thread_set_state: 0
  Calls made by all processes on this machine:
    task_for_pid: 0
    thread_create: 0
    thread_set_state: 0

VM Region Summary:
ReadOnly portion of Libraries: Total=1.7G resident=0K(0%) swapped_out_or_unallocated=1.7G(100%)
Writable regions: Total=2.1G written=1733K(0%) resident=1285K(0%) swapped_out=448K(0%) unallocated=2.1G(100%)

                                VIRTUAL   REGION 
REGION TYPE                        SIZE    COUNT (non-coalesced) 
===========                     =======  ======= 
Accelerate framework               128K        1 
Activity Tracing                   256K        1 
CG image                          2240K       23 
ColorSync                          656K       35 
CoreAnimation                     1968K      104 
CoreGraphics                        64K        4 
CoreUI image data                 5360K       42 
Foundation                          48K        2 
Image IO                            32K        2 
JS VM Gigacage                    4096K        1 
JS VM Isolated Heap               6464K        5 
Kernel Alloc Once                   32K        1 
MALLOC                             2.1G      125 
MALLOC guard page                  288K       18 
STACK GUARD                       56.5M       29 
Stack                             22.9M       30 
VM_ALLOCATE                        512K       23 
__AUTH                            5446K      688 
__AUTH_CONST                      76.4M      928 
__CTF                               824        1 
__DATA                            26.3M      959 
__DATA_CONST                      28.9M      990 
__DATA_DIRTY                      2763K      336 
__FONT_DATA                        2352        1 
__INFO_FILTER                         8        1 
__LINKEDIT                       637.9M       57 
__OBJC_RO                         61.4M        1 
__OBJC_RW                         2396K        1 
__TEXT                             1.1G     1012 
__TPRO_CONST                       128K        2 
dyld private memory                128K        1 
mapped file                      592.6M       73 
page table in kernel              1285K        1 
shared memory                     1392K       14 
===========                     =======  ======= 
TOTAL                              4.7G     5512 



-----------
Full Report
-----------

{"app_name":"qlcplus","timestamp":"2025-12-01 20:06:11.00 +0100","app_version":"","slice_uuid":"ff96842b-07be-35bd-a984-202ce8519030","build_version":"","platform":1,"share_with_app_devs":0,"is_first_party":1,"bug_type":"309","os_version":"macOS 15.6.1 (24G90)","roots_installed":0,"incident_id":"3FC626CB-8696-4136-8961-D1D09F3796AA","name":"qlcplus"}
{
  "uptime" : 66000,
  "procRole" : "Foreground",
  "version" : 2,
  "userID" : 501,
  "deployVersion" : 210,
  "modelCode" : "MacBookPro18,2",
  "coalitionID" : 10775,
  "osVersion" : {
    "train" : "macOS 15.6.1",
    "build" : "24G90",
    "releaseType" : "User"
  },
  "captureTime" : "2025-12-01 20:05:56.5074 +0100",
  "codeSigningMonitor" : 1,
  "incident" : "3FC626CB-8696-4136-8961-D1D09F3796AA",
  "pid" : 52024,
  "translated" : false,
  "cpuType" : "ARM-64",
  "roots_installed" : 0,
  "bug_type" : "309",
  "procLaunch" : "2025-12-01 16:23:46.7967 +0100",
  "procStartAbsTime" : 1538263227883,
  "procExitAbsTime" : 1585807081948,
  "procName" : "qlcplus",
  "procPath" : "\/Users\/USER\/QLC+.app\/Contents\/MacOS\/qlcplus",
  "parentProc" : "launchd",
  "parentPid" : 1,
  "crashReporterKey" : "4944AA08-7DDB-B375-C327-A7109C14C831",
  "appleIntelligenceStatus" : {"state":"available"},
  "responsiblePid" : 52022,
  "codeSigningID" : "qlcplus-55554944ff96842b07be35bda984202ce8519030",
  "codeSigningTeamID" : "",
  "codeSigningFlags" : 570425857,
  "codeSigningValidationCategory" : 10,
  "codeSigningTrustLevel" : 4294967295,
  "codeSigningAuxiliaryInfo" : 0,
  "instructionByteStream" : {"beforePC":"fwIW64AAAFRpIkC5vwIJa6oHAFT\/\/wGpAwAAFOgCQPlIBAC09wMIqg==","atPC":"CCFAub8CCGtr\/\/9UHwEVayoIAFToBkD5KP\/\/tfYiAJEZAAAU6AcA+Q=="},
  "bootSessionUUID" : "0BE058C9-B959-4434-96C1-FB4D5713322B",
  "wakeTime" : 52,
  "sleepWakeUUID" : "7C76A1CE-BFE6-4485-BDE6-2569193781E3",
  "sip" : "enabled",
  "vmRegionInfo" : "0x112 is not in any region.  Bytes before following region: 4329209582\n      REGION TYPE                    START - END         [ VSIZE] PRT\/MAX SHRMOD  REGION DETAIL\n      UNUSED SPACE AT START\n--->  \n      __TEXT                      1020a8000-1020b4000    [   48K] r-x\/r-x SM=COW  \/Users\/USER\/QLC+.app\/Contents\/MacOS\/qlcplus",
  "exception" : {"codes":"0x0000000000000001, 0x0000000000000112","rawCodes":[1,274],"type":"EXC_BAD_ACCESS","signal":"SIGSEGV","subtype":"KERN_INVALID_ADDRESS at 0x0000000000000112"},
  "termination" : {"flags":0,"code":11,"namespace":"SIGNAL","indicator":"Segmentation fault: 11","byProc":"exc handler","byPid":52024},
  "vmregioninfo" : "0x112 is not in any region.  Bytes before following region: 4329209582\n      REGION TYPE                    START - END         [ VSIZE] PRT\/MAX SHRMOD  REGION DETAIL\n      UNUSED SPACE AT START\n--->  \n      __TEXT                      1020a8000-1020b4000    [   48K] r-x\/r-x SM=COW  \/Users\/USER\/QLC+.app\/Contents\/MacOS\/qlcplus",
  "extMods" : {"caller":{"thread_create":0,"thread_set_state":0,"task_for_pid":0},"system":{"thread_create":0,"thread_set_state":0,"task_for_pid":0},"targeted":{"thread_create":0,"thread_set_state":0,"task_for_pid":0},"warnings":0},
  "faultingThread" : 0,
  "threads" : [{"triggered":true,"id":1467803,"threadState":{"x":[{"value":5175763680},{"value":6137664132},{"value":76},{"value":0},{"value":6137664152},{"value":2487478407},{"value":105553123565952},{"value":99997000},{"value":242},{"value":0},{"value":105553116453768},{"value":105553116453768},{"value":105553140047608},{"value":51},{"value":5795447808},{"value":2147496697},{"value":4334160324,"symbolLocation":0,"symbol":"Function::requestAttributeOverride(int, double)"},{"value":135},{"value":0},{"value":105553146221008},{"value":105553146220992},{"value":132},{"value":105553146221008},{"value":242},{"value":4530950120},{"value":4530950112},{"value":0},{"value":144},{"value":1}],"flavor":"ARM_THREAD_STATE64","lr":{"value":4334162168},"cpsr":{"value":2147487744},"fp":{"value":6137664064},"sp":{"value":6137663968},"esr":{"value":2449473542,"description":"(Data Abort) byte read Translation fault"},"pc":{"value":4334162264,"matchesCrashFrame":1},"far":{"value":274}},"queue":"com.apple.main-thread","frames":[{"imageOffset":495960,"symbol":"QMap<int, AttributeOverride>::operator[](int const&)","symbolLocation":180,"imageIndex":3},{"imageOffset":494144,"symbol":"Function::requestAttributeOverride(int, double)","symbolLocation":124,"imageIndex":3},{"imageOffset":2061052,"symbol":"VCXYPad::slotPresetClicked(bool)","symbolLocation":2208,"imageIndex":2},{"imageOffset":962028,"symbol":"void doActivate<false>(QObject*, int, void**)","symbolLocation":1480,"imageIndex":12},{"imageOffset":1083840,"symbol":"QAbstractButtonPrivate::emitClicked()","symbolLocation":112,"imageIndex":6},{"imageOffset":1082692,"symbol":"QAbstractButton::click()","symbolLocation":136,"imageIndex":6},{"imageOffset":962028,"symbol":"void doActivate<false>(QObject*, int, void**)","symbolLocation":1480,"imageIndex":12},{"imageOffset":53276,"symbol":"InputOutputMap::inputValueChanged(unsigned int, unsigned int, unsigned char, QString const&)","symbolLocation":80,"imageIndex":3},{"imageOffset":929104,"symbol":"QObject::event(QEvent*)","symbolLocation":324,"imageIndex":12},{"imageOffset":51036,"symbol":"QApplicationPrivate::notify_helper(QObject*, QEvent*)","symbolLocation":336,"imageIndex":6},{"imageOffset":55152,"symbol":"QApplication::notify(QObject*, QEvent*)","symbolLocation":468,"imageIndex":6},{"imageOffset":636540,"symbol":"QCoreApplicationPrivate::sendPostedEvents(QObject*, int, QThreadData*)","symbolLocation":628,"imageIndex":12},{"imageOffset":92348,"symbol":"QCocoaEventDispatcherPrivate::processPostedEvents()","symbolLocation":380,"imageIndex":36},{"imageOffset":97140,"symbol":"QCocoaEventDispatcherPrivate::postedEventsSourceCallback(void*)","symbolLocation":516,"imageIndex":36},{"imageOffset":510740,"symbol":"__CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__","symbolLocation":28,"imageIndex":56},{"imageOffset":510632,"symbol":"__CFRunLoopDoSource0","symbolLocation":172,"imageIndex":56},{"imageOffset":509972,"symbol":"__CFRunLoopDoSources0","symbolLocation":232,"imageIndex":56},{"imageOffset":504936,"symbol":"__CFRunLoopRun","symbolLocation":840,"imageIndex":56},{"imageOffset":502424,"symbol":"CFRunLoopRunSpecific","symbolLocation":572,"imageIndex":56},{"imageOffset":799356,"symbol":"RunCurrentEventLoopInMode","symbolLocation":324,"imageIndex":57},{"imageOffset":812264,"symbol":"ReceiveNextEventCommon","symbolLocation":676,"imageIndex":57},{"imageOffset":2430084,"symbol":"_BlockUntilNextEventMatchingListInModeWithFilter","symbolLocation":76,"imageIndex":57},{"imageOffset":240180,"symbol":"_DPSNextEvent","symbolLocation":684,"imageIndex":58},{"imageOffset":10328384,"symbol":"-[NSApplication(NSEventRouting) _nextEventMatchingEventMask:untilDate:inMode:dequeue:]","symbolLocation":688,"imageIndex":58},{"imageOffset":187364,"symbol":"-[NSApplication run]","symbolLocation":480,"imageIndex":58},{"imageOffset":86944,"symbol":"QCocoaEventDispatcher::processEvents(QFlags<QEventLoop::ProcessEventsFlag>)","symbolLocation":2152,"imageIndex":36},{"imageOffset":670076,"symbol":"QEventLoop::exec(QFlags<QEventLoop::ProcessEventsFlag>)","symbolLocation":588,"imageIndex":12},{"imageOffset":633504,"symbol":"QCoreApplication::exec()","symbolLocation":228,"imageIndex":12},{"imageOffset":39252,"symbol":"main","symbolLocation":1156,"imageIndex":0},{"imageOffset":27544,"symbol":"start","symbolLocation":6076,"imageIndex":59}]},{"id":1467815,"name":"com.apple.NSEventThread","threadState":{"x":[{"value":268451845},{"value":21592279046},{"value":8589934592,"symbolLocation":116,"symbol":"ControllerImpl::alertFinished(AVVoiceController*)"},{"value":190228396507136},{"value":0},{"value":190228396507136},{"value":2},{"value":4294967295},{"value":0},{"value":17179869184},{"value":0},{"value":2},{"value":0},{"value":0},{"value":44291},{"value":0},{"value":18446744073709551569},{"value":8830131304},{"value":0},{"value":4294967295},{"value":2},{"value":190228396507136},{"value":0},{"value":190228396507136},{"value":6140518536},{"value":8589934592,"symbolLocation":116,"symbol":"ControllerImpl::alertFinished(AVVoiceController*)"},{"value":21592279046},{"value":18446744073709550527},{"value":4412409862}],"flavor":"ARM_THREAD_STATE64","lr":{"value":6967755680},"cpsr":{"value":4096},"fp":{"value":6140518384},"sp":{"value":6140518304},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967680052},"far":{"value":0}},"frames":[{"imageOffset":3124,"symbol":"mach_msg2_trap","symbolLocation":8,"imageIndex":61},{"imageOffset":78752,"symbol":"mach_msg2_internal","symbolLocation":76,"imageIndex":61},{"imageOffset":38756,"symbol":"mach_msg_overwrite","symbolLocation":484,"imageIndex":61},{"imageOffset":4008,"symbol":"mach_msg","symbolLocation":24,"imageIndex":61},{"imageOffset":511164,"symbol":"__CFRunLoopServiceMachPort","symbolLocation":160,"imageIndex":56},{"imageOffset":505304,"symbol":"__CFRunLoopRun","symbolLocation":1208,"imageIndex":56},{"imageOffset":502424,"symbol":"CFRunLoopRunSpecific","symbolLocation":572,"imageIndex":56},{"imageOffset":1435532,"symbol":"_NSEventThread","symbolLocation":140,"imageIndex":58},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467819,"name":"HPMPrivate","threadState":{"x":[{"value":268451845},{"value":21592279046},{"value":8589934592,"symbolLocation":116,"symbol":"ControllerImpl::alertFinished(AVVoiceController*)"},{"value":235617610891264},{"value":0},{"value":235617610891264},{"value":2},{"value":4294967295},{"value":0},{"value":17179869184},{"value":0},{"value":2},{"value":0},{"value":0},{"value":54859},{"value":2043},{"value":18446744073709551569},{"value":27},{"value":0},{"value":4294967295},{"value":2},{"value":235617610891264},{"value":0},{"value":235617610891264},{"value":6141091720},{"value":8589934592,"symbolLocation":116,"symbol":"ControllerImpl::alertFinished(AVVoiceController*)"},{"value":21592279046},{"value":18446744073709550527},{"value":4412409862}],"flavor":"ARM_THREAD_STATE64","lr":{"value":6967755680},"cpsr":{"value":4096},"fp":{"value":6141091568},"sp":{"value":6141091488},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967680052},"far":{"value":0}},"frames":[{"imageOffset":3124,"symbol":"mach_msg2_trap","symbolLocation":8,"imageIndex":61},{"imageOffset":78752,"symbol":"mach_msg2_internal","symbolLocation":76,"imageIndex":61},{"imageOffset":38756,"symbol":"mach_msg_overwrite","symbolLocation":484,"imageIndex":61},{"imageOffset":4008,"symbol":"mach_msg","symbolLocation":24,"imageIndex":61},{"imageOffset":511164,"symbol":"__CFRunLoopServiceMachPort","symbolLocation":160,"imageIndex":56},{"imageOffset":505304,"symbol":"__CFRunLoopRun","symbolLocation":1208,"imageIndex":56},{"imageOffset":502424,"symbol":"CFRunLoopRunSpecific","symbolLocation":572,"imageIndex":56},{"imageOffset":1000788,"symbol":"CFRunLoopRun","symbolLocation":64,"imageIndex":56},{"imageOffset":1204720,"symbol":"HPMPrivate::run()","symbolLocation":264,"imageIndex":3},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467820,"name":"com.apple.CFSocket.private","threadState":{"x":[{"value":4},{"value":0},{"value":105553145513184},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":6141669600},{"value":0},{"value":5661161591},{"value":31},{"value":7},{"value":5472010576},{"value":72057602851031921,"symbolLocation":72057594037927937,"symbol":"OBJC_CLASS_$___NSCFArray"},{"value":8813103984,"symbolLocation":0,"symbol":"OBJC_CLASS_$___NSCFArray"},{"value":93},{"value":8830133640},{"value":0},{"value":8813110472,"symbolLocation":0,"symbol":"__CFActiveSocketsLock"},{"value":32},{"value":2},{"value":105553145513184},{"value":8834040448,"symbolLocation":0,"symbol":"__kCFNull"},{"value":0},{"value":105553141067488},{"value":105553145512288},{"value":105553141063360},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":6969068336},"cpsr":{"value":1610616832},"fp":{"value":6141669312},"sp":{"value":6141635536},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967725100},"far":{"value":0}},"frames":[{"imageOffset":48172,"symbol":"__select","symbolLocation":8,"imageIndex":61},{"imageOffset":666416,"symbol":"__CFSocketManager","symbolLocation":704,"imageIndex":56},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467822,"frames":[{"imageOffset":3124,"symbol":"mach_msg2_trap","symbolLocation":8,"imageIndex":61},{"imageOffset":78752,"symbol":"mach_msg2_internal","symbolLocation":76,"imageIndex":61},{"imageOffset":38756,"symbol":"mach_msg_overwrite","symbolLocation":484,"imageIndex":61},{"imageOffset":4008,"symbol":"mach_msg","symbolLocation":24,"imageIndex":61},{"imageOffset":257708,"symbol":"XServerMachPort::ReceiveMessage(int&, void*, int&)","symbolLocation":104,"imageIndex":64},{"imageOffset":395072,"symbol":"MIDIProcess::MIDIInPortThread::Run()","symbolLocation":148,"imageIndex":64},{"imageOffset":350060,"symbol":"CADeprecated::XThread::RunHelper(void*)","symbolLocation":48,"imageIndex":64},{"imageOffset":392772,"symbol":"CADeprecated::CAPThread::Entry(CADeprecated::CAPThread*)","symbolLocation":96,"imageIndex":64},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}],"threadState":{"x":[{"value":268451845},{"value":17179869186},{"value":0},{"value":428822419734528},{"value":0},{"value":428822419734528},{"value":100},{"value":0},{"value":0},{"value":17179869184},{"value":100},{"value":0},{"value":0},{"value":0},{"value":99843},{"value":0},{"value":18446744073709551569},{"value":8830131304},{"value":0},{"value":0},{"value":100},{"value":428822419734528},{"value":0},{"value":428822419734528},{"value":6142241236},{"value":0},{"value":17179869186},{"value":18446744073709550527},{"value":2}],"flavor":"ARM_THREAD_STATE64","lr":{"value":6967755680},"cpsr":{"value":4096},"fp":{"value":6142240896},"sp":{"value":6142240816},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967680052},"far":{"value":0}}},{"id":1467837,"name":"org.libusb.device-hotplug","threadState":{"x":[{"value":268451845},{"value":21592279046},{"value":8589934592,"symbolLocation":116,"symbol":"ControllerImpl::alertFinished(AVVoiceController*)"},{"value":505788233678848},{"value":0},{"value":505788233678848},{"value":2},{"value":4294967295},{"value":0},{"value":17179869184},{"value":0},{"value":2},{"value":0},{"value":0},{"value":117763},{"value":2043},{"value":18446744073709551569},{"value":4},{"value":0},{"value":4294967295},{"value":2},{"value":505788233678848},{"value":0},{"value":505788233678848},{"value":6143385528},{"value":8589934592,"symbolLocation":116,"symbol":"ControllerImpl::alertFinished(AVVoiceController*)"},{"value":21592279046},{"value":18446744073709550527},{"value":4412409862}],"flavor":"ARM_THREAD_STATE64","lr":{"value":6967755680},"cpsr":{"value":4096},"fp":{"value":6143385376},"sp":{"value":6143385296},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967680052},"far":{"value":0}},"frames":[{"imageOffset":3124,"symbol":"mach_msg2_trap","symbolLocation":8,"imageIndex":61},{"imageOffset":78752,"symbol":"mach_msg2_internal","symbolLocation":76,"imageIndex":61},{"imageOffset":38756,"symbol":"mach_msg_overwrite","symbolLocation":484,"imageIndex":61},{"imageOffset":4008,"symbol":"mach_msg","symbolLocation":24,"imageIndex":61},{"imageOffset":511164,"symbol":"__CFRunLoopServiceMachPort","symbolLocation":160,"imageIndex":56},{"imageOffset":505304,"symbol":"__CFRunLoopRun","symbolLocation":1208,"imageIndex":56},{"imageOffset":502424,"symbol":"CFRunLoopRunSpecific","symbolLocation":572,"imageIndex":56},{"imageOffset":1000788,"symbol":"CFRunLoopRun","symbolLocation":64,"imageIndex":56},{"imageOffset":50272,"symbol":"darwin_event_thread_main","symbolLocation":408,"imageIndex":43},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467838,"name":"caulk.messenger.shared:17","threadState":{"x":[{"value":14},{"value":105553143460570},{"value":0},{"value":6143963242},{"value":105553143460544},{"value":25},{"value":0},{"value":0},{"value":0},{"value":4294967295},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":18446744073709551580},{"value":8830133736},{"value":0},{"value":105553152533680},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":7159262408},"cpsr":{"value":2147487744},"fp":{"value":6143963008},"sp":{"value":6143962976},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967679920},"far":{"value":0}},"frames":[{"imageOffset":2992,"symbol":"semaphore_wait_trap","symbolLocation":8,"imageIndex":61},{"imageOffset":7368,"symbol":"caulk::semaphore::timed_wait(double)","symbolLocation":224,"imageIndex":65},{"imageOffset":7024,"symbol":"caulk::concurrent::details::worker_thread::run()","symbolLocation":32,"imageIndex":65},{"imageOffset":6212,"symbol":"void* caulk::thread_proxy<std::__1::tuple<caulk::thread::attributes, void (caulk::concurrent::details::worker_thread::*)(), std::__1::tuple<caulk::concurrent::details::worker_thread*>>>(void*)","symbolLocation":96,"imageIndex":65},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467839,"name":"caulk.messenger.shared:high","threadState":{"x":[{"value":14},{"value":105553143494204},{"value":0},{"value":6144536684},{"value":105553143494176},{"value":27},{"value":0},{"value":0},{"value":0},{"value":4294967295},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":18446744073709551580},{"value":8830133736},{"value":0},{"value":105553152534096},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":7159262408},"cpsr":{"value":2147487744},"fp":{"value":6144536448},"sp":{"value":6144536416},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967679920},"far":{"value":0}},"frames":[{"imageOffset":2992,"symbol":"semaphore_wait_trap","symbolLocation":8,"imageIndex":61},{"imageOffset":7368,"symbol":"caulk::semaphore::timed_wait(double)","symbolLocation":224,"imageIndex":65},{"imageOffset":7024,"symbol":"caulk::concurrent::details::worker_thread::run()","symbolLocation":32,"imageIndex":65},{"imageOffset":6212,"symbol":"void* caulk::thread_proxy<std::__1::tuple<caulk::thread::attributes, void (caulk::concurrent::details::worker_thread::*)(), std::__1::tuple<caulk::concurrent::details::worker_thread*>>>(void*)","symbolLocation":96,"imageIndex":65},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467915,"name":"QThread","threadState":{"x":[{"value":4},{"value":0},{"value":1},{"value":1},{"value":0},{"value":11254500},{"value":52},{"value":0},{"value":8813032344,"symbolLocation":0,"symbol":"clock_sem"},{"value":16387},{"value":17},{"value":66075},{"value":4294967293},{"value":697071680312311808},{"value":162299648},{"value":0},{"value":334},{"value":8830129544},{"value":0},{"value":105553145577936},{"value":105553145577968},{"value":105553145577904},{"value":105553145577968},{"value":105553145577936},{"value":105553165322304},{"value":6147403460},{"value":20000000},{"value":105553381825856},{"value":1000000000}],"flavor":"ARM_THREAD_STATE64","lr":{"value":6966499060},"cpsr":{"value":1610616832},"fp":{"value":6147403440},"sp":{"value":6147403392},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967693768},"far":{"value":0}},"frames":[{"imageOffset":16840,"symbol":"__semwait_signal","symbolLocation":8,"imageIndex":61},{"imageOffset":55028,"symbol":"nanosleep","symbolLocation":220,"imageIndex":66},{"imageOffset":1065728,"symbol":"MasterTimerPrivate::run()","symbolLocation":620,"imageIndex":3},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467917,"name":"QThread","threadState":{"x":[{"value":4},{"value":0},{"value":4294967295},{"value":4294967296},{"value":105553240765568},{"value":4334765868,"symbolLocation":0,"symbol":"QtPrivate::QCallableObject<RGBScript::paramCount() const::$_0, QtPrivate::List<>, int>::impl(int, QtPrivate::QSlotObjectBase*, QObject*, void**, bool*)"},{"value":0},{"value":210},{"value":9223372036854775807},{"value":1},{"value":1},{"value":0},{"value":2043},{"value":2045},{"value":3898820713},{"value":3896721456},{"value":230},{"value":8830131176},{"value":0},{"value":1},{"value":105553142719408},{"value":9223372036854775807},{"value":4294967296},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":4387246696},"cpsr":{"value":1610616832},"fp":{"value":6145109456},"sp":{"value":6145109392},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967714968},"far":{"value":0}},"frames":[{"imageOffset":38040,"symbol":"poll","symbolLocation":8,"imageIndex":61},{"imageOffset":2232936,"symbol":"qt_safe_poll(pollfd*, unsigned int, QDeadlineTimer)","symbolLocation":76,"imageIndex":12},{"imageOffset":2249164,"symbol":"QEventDispatcherUNIX::processEvents(QFlags<QEventLoop::ProcessEventsFlag>)","symbolLocation":880,"imageIndex":12},{"imageOffset":670076,"symbol":"QEventLoop::exec(QFlags<QEventLoop::ProcessEventsFlag>)","symbolLocation":588,"imageIndex":12},{"imageOffset":1640336,"symbol":"QThread::exec()","symbolLocation":336,"imageIndex":12},{"imageOffset":1089860,"symbol":"JSThread::run()","symbolLocation":56,"imageIndex":3},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467926,"name":"Universe","threadState":{"x":[{"value":18446744073709551612},{"value":0},{"value":2147483648},{"value":39999667},{"value":0},{"value":4526957664,"symbolLocation":0,"symbol":"ArtNetPlugin::writeUniverse(unsigned int, unsigned int, QByteArray const&, bool)"},{"value":0},{"value":0},{"value":39999667},{"value":973582291},{"value":1000000000},{"value":40000000},{"value":105553169269752},{"value":127},{"value":162298368},{"value":0},{"value":544},{"value":8830131176},{"value":0},{"value":1},{"value":4420827728},{"value":2147483648},{"value":1},{"value":2147483648},{"value":2147483648},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":4387284932},"cpsr":{"value":4096},"fp":{"value":6145683040},"sp":{"value":6145682928},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967736916},"far":{"value":0}},"frames":[{"imageOffset":59988,"symbol":"__ulock_wait2","symbolLocation":8,"imageIndex":61},{"imageOffset":2271172,"symbol":"bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer)","symbolLocation":260,"imageIndex":12},{"imageOffset":2270468,"symbol":"QSemaphore::tryAcquire(int, QDeadlineTimer)","symbolLocation":208,"imageIndex":12},{"imageOffset":1013424,"symbol":"Universe::run()","symbolLocation":284,"imageIndex":3},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467927,"name":"Universe","threadState":{"x":[{"value":18446744073709551612},{"value":0},{"value":2147483648},{"value":39999750},{"value":0},{"value":22},{"value":0},{"value":0},{"value":39999750},{"value":973595750},{"value":1000000000},{"value":40000000},{"value":2043},{"value":2045},{"value":2566936735},{"value":2564837536},{"value":544},{"value":8830131176},{"value":0},{"value":1},{"value":4420826416},{"value":2147483648},{"value":1},{"value":2147483648},{"value":2147483648},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":4387284932},"cpsr":{"value":4096},"fp":{"value":6146256480},"sp":{"value":6146256368},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967736916},"far":{"value":0}},"frames":[{"imageOffset":59988,"symbol":"__ulock_wait2","symbolLocation":8,"imageIndex":61},{"imageOffset":2271172,"symbol":"bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer)","symbolLocation":260,"imageIndex":12},{"imageOffset":2270468,"symbol":"QSemaphore::tryAcquire(int, QDeadlineTimer)","symbolLocation":208,"imageIndex":12},{"imageOffset":1013424,"symbol":"Universe::run()","symbolLocation":284,"imageIndex":3},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467928,"name":"Universe","threadState":{"x":[{"value":18446744073709551612},{"value":0},{"value":2147483648},{"value":39999833},{"value":0},{"value":0},{"value":0},{"value":0},{"value":39999833},{"value":973606458},{"value":1000000000},{"value":40000000},{"value":4294967293},{"value":697071680312311808},{"value":162299648},{"value":0},{"value":544},{"value":8830131176},{"value":0},{"value":1},{"value":4530922032},{"value":2147483648},{"value":1},{"value":2147483648},{"value":2147483648},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":4387284932},"cpsr":{"value":4096},"fp":{"value":6146829920},"sp":{"value":6146829808},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967736916},"far":{"value":0}},"frames":[{"imageOffset":59988,"symbol":"__ulock_wait2","symbolLocation":8,"imageIndex":61},{"imageOffset":2271172,"symbol":"bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer)","symbolLocation":260,"imageIndex":12},{"imageOffset":2270468,"symbol":"QSemaphore::tryAcquire(int, QDeadlineTimer)","symbolLocation":208,"imageIndex":12},{"imageOffset":1013424,"symbol":"Universe::run()","symbolLocation":284,"imageIndex":3},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467929,"name":"Universe","threadState":{"x":[{"value":18446744073709551612},{"value":0},{"value":2147483648},{"value":39999833},{"value":0},{"value":18},{"value":0},{"value":0},{"value":39999833},{"value":973611333},{"value":1000000000},{"value":40000000},{"value":4530913132},{"value":4330375712},{"value":4530913408},{"value":203744},{"value":544},{"value":8830131176},{"value":0},{"value":1},{"value":5175426304},{"value":2147483648},{"value":1},{"value":2147483648},{"value":2147483648},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":4387284932},"cpsr":{"value":4096},"fp":{"value":6147976800},"sp":{"value":6147976688},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967736916},"far":{"value":0}},"frames":[{"imageOffset":59988,"symbol":"__ulock_wait2","symbolLocation":8,"imageIndex":61},{"imageOffset":2271172,"symbol":"bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer)","symbolLocation":260,"imageIndex":12},{"imageOffset":2270468,"symbol":"QSemaphore::tryAcquire(int, QDeadlineTimer)","symbolLocation":208,"imageIndex":12},{"imageOffset":1013424,"symbol":"Universe::run()","symbolLocation":284,"imageIndex":3},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467930,"name":"Universe","threadState":{"x":[{"value":18446744073709551612},{"value":0},{"value":2147483648},{"value":39999875},{"value":0},{"value":4525644792,"symbolLocation":0,"symbol":"Loopback::writeUniverse(unsigned int, unsigned int, QByteArray const&, bool)"},{"value":0},{"value":0},{"value":39999875},{"value":973607416},{"value":1000000000},{"value":40000000},{"value":105553140610848},{"value":105553140610856},{"value":536870912},{"value":536870912},{"value":544},{"value":8830131176},{"value":0},{"value":1},{"value":5175514608},{"value":2147483648},{"value":1},{"value":2147483648},{"value":2147483648},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":4387284932},"cpsr":{"value":4096},"fp":{"value":6148550240},"sp":{"value":6148550128},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967736916},"far":{"value":0}},"frames":[{"imageOffset":59988,"symbol":"__ulock_wait2","symbolLocation":8,"imageIndex":61},{"imageOffset":2271172,"symbol":"bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer)","symbolLocation":260,"imageIndex":12},{"imageOffset":2270468,"symbol":"QSemaphore::tryAcquire(int, QDeadlineTimer)","symbolLocation":208,"imageIndex":12},{"imageOffset":1013424,"symbol":"Universe::run()","symbolLocation":284,"imageIndex":3},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467931,"name":"Universe","threadState":{"x":[{"value":18446744073709551612},{"value":0},{"value":2147483648},{"value":39999833},{"value":0},{"value":4525644792,"symbolLocation":0,"symbol":"Loopback::writeUniverse(unsigned int, unsigned int, QByteArray const&, bool)"},{"value":105553162866816},{"value":0},{"value":39999833},{"value":973589458},{"value":1000000000},{"value":40000000},{"value":2043},{"value":2045},{"value":3233951767},{"value":3231852568},{"value":544},{"value":8830131176},{"value":0},{"value":1},{"value":5175504304},{"value":2147483648},{"value":1},{"value":2147483648},{"value":2147483648},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":4387284932},"cpsr":{"value":4096},"fp":{"value":6149123680},"sp":{"value":6149123568},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967736916},"far":{"value":0}},"frames":[{"imageOffset":59988,"symbol":"__ulock_wait2","symbolLocation":8,"imageIndex":61},{"imageOffset":2271172,"symbol":"bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer)","symbolLocation":260,"imageIndex":12},{"imageOffset":2270468,"symbol":"QSemaphore::tryAcquire(int, QDeadlineTimer)","symbolLocation":208,"imageIndex":12},{"imageOffset":1013424,"symbol":"Universe::run()","symbolLocation":284,"imageIndex":3},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467932,"name":"Universe","threadState":{"x":[{"value":18446744073709551612},{"value":0},{"value":2147483648},{"value":39999833},{"value":0},{"value":22},{"value":0},{"value":0},{"value":39999833},{"value":973597458},{"value":1000000000},{"value":40000000},{"value":2043},{"value":2045},{"value":2443337822},{"value":2441238623},{"value":544},{"value":8830131176},{"value":0},{"value":1},{"value":5175500704},{"value":2147483648},{"value":1},{"value":2147483648},{"value":2147483648},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":4387284932},"cpsr":{"value":4096},"fp":{"value":6149697120},"sp":{"value":6149697008},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967736916},"far":{"value":0}},"frames":[{"imageOffset":59988,"symbol":"__ulock_wait2","symbolLocation":8,"imageIndex":61},{"imageOffset":2271172,"symbol":"bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer)","symbolLocation":260,"imageIndex":12},{"imageOffset":2270468,"symbol":"QSemaphore::tryAcquire(int, QDeadlineTimer)","symbolLocation":208,"imageIndex":12},{"imageOffset":1013424,"symbol":"Universe::run()","symbolLocation":284,"imageIndex":3},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467933,"name":"Universe","threadState":{"x":[{"value":18446744073709551612},{"value":0},{"value":2147483648},{"value":39999875},{"value":0},{"value":22},{"value":0},{"value":0},{"value":39999875},{"value":973601416},{"value":1000000000},{"value":40000000},{"value":2043},{"value":2045},{"value":2579519647},{"value":2577420448},{"value":544},{"value":8830131176},{"value":0},{"value":1},{"value":5175416128},{"value":2147483648},{"value":1},{"value":2147483648},{"value":2147483648},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":4387284932},"cpsr":{"value":4096},"fp":{"value":6150270560},"sp":{"value":6150270448},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967736916},"far":{"value":0}},"frames":[{"imageOffset":59988,"symbol":"__ulock_wait2","symbolLocation":8,"imageIndex":61},{"imageOffset":2271172,"symbol":"bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer)","symbolLocation":260,"imageIndex":12},{"imageOffset":2270468,"symbol":"QSemaphore::tryAcquire(int, QDeadlineTimer)","symbolLocation":208,"imageIndex":12},{"imageOffset":1013424,"symbol":"Universe::run()","symbolLocation":284,"imageIndex":3},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1467934,"name":"Universe","threadState":{"x":[{"value":18446744073709551612},{"value":0},{"value":2147483648},{"value":39999875},{"value":0},{"value":22},{"value":0},{"value":0},{"value":39999875},{"value":973602333},{"value":1000000000},{"value":40000000},{"value":2043},{"value":2045},{"value":2361731077},{"value":2359631878},{"value":544},{"value":8830131176},{"value":0},{"value":1},{"value":5175413392},{"value":2147483648},{"value":1},{"value":2147483648},{"value":2147483648},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":4387284932},"cpsr":{"value":4096},"fp":{"value":6150844000},"sp":{"value":6150843888},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967736916},"far":{"value":0}},"frames":[{"imageOffset":59988,"symbol":"__ulock_wait2","symbolLocation":8,"imageIndex":61},{"imageOffset":2271172,"symbol":"bool futexSemaphoreTryAcquire<QDeadlineTimer>(QBasicAtomicInteger<unsigned long long>&, int, QDeadlineTimer)","symbolLocation":260,"imageIndex":12},{"imageOffset":2270468,"symbol":"QSemaphore::tryAcquire(int, QDeadlineTimer)","symbolLocation":208,"imageIndex":12},{"imageOffset":1013424,"symbol":"Universe::run()","symbolLocation":284,"imageIndex":3},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1503096,"frames":[{"imageOffset":7020,"symbol":"start_wqthread","symbolLocation":0,"imageIndex":62}],"threadState":{"x":[{"value":6139375616},{"value":261407},{"value":6138839040},{"value":0},{"value":409604},{"value":18446744073709551615},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":0},"cpsr":{"value":4096},"fp":{"value":0},"sp":{"value":6139375616},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967929708},"far":{"value":0}}},{"id":1513421,"frames":[{"imageOffset":7020,"symbol":"start_wqthread","symbolLocation":0,"imageIndex":62}],"threadState":{"x":[{"value":6139949056},{"value":60667},{"value":6139412480},{"value":0},{"value":409604},{"value":18446744073709551615},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":0},"cpsr":{"value":4096},"fp":{"value":0},"sp":{"value":6139949056},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967929708},"far":{"value":0}}},{"id":1514448,"name":"Thread (pooled)","threadState":{"x":[{"value":260},{"value":0},{"value":5947904},{"value":0},{"value":0},{"value":160},{"value":29},{"value":999998000},{"value":6142815608},{"value":0},{"value":5120},{"value":21990232560642},{"value":21990232560642},{"value":5120},{"value":0},{"value":21990232560640},{"value":305},{"value":8830129424},{"value":0},{"value":105553174760320},{"value":105553174760384},{"value":6142816480},{"value":999998000},{"value":29},{"value":5947904},{"value":5947905},{"value":5948160},{"value":1},{"value":5195015488}],"flavor":"ARM_THREAD_STATE64","lr":{"value":6967951584},"cpsr":{"value":1610616832},"fp":{"value":6142815728},"sp":{"value":6142815584},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967694284},"far":{"value":0}},"frames":[{"imageOffset":17356,"symbol":"__psynch_cvwait","symbolLocation":8,"imageIndex":61},{"imageOffset":28896,"symbol":"_pthread_cond_wait","symbolLocation":984,"imageIndex":62},{"imageOffset":2295220,"symbol":"QWaitConditionPrivate::wait(QDeadlineTimer)","symbolLocation":236,"imageIndex":12},{"imageOffset":2294880,"symbol":"QWaitCondition::wait(QMutex*, QDeadlineTimer)","symbolLocation":108,"imageIndex":12},{"imageOffset":2272456,"symbol":"QThreadPoolThread::run()","symbolLocation":932,"imageIndex":12},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1514449,"name":"Thread (pooled)","threadState":{"x":[{"value":260},{"value":0},{"value":5974272},{"value":0},{"value":0},{"value":160},{"value":29},{"value":999999000},{"value":6151417208},{"value":0},{"value":4864},{"value":20890720932610},{"value":20890720932610},{"value":4864},{"value":0},{"value":20890720932608},{"value":305},{"value":8830129424},{"value":0},{"value":105553174760704},{"value":105553174760768},{"value":6151418080},{"value":999999000},{"value":29},{"value":5974272},{"value":5974273},{"value":5974528},{"value":1},{"value":5195015488}],"flavor":"ARM_THREAD_STATE64","lr":{"value":6967951584},"cpsr":{"value":1610616832},"fp":{"value":6151417328},"sp":{"value":6151417184},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967694284},"far":{"value":0}},"frames":[{"imageOffset":17356,"symbol":"__psynch_cvwait","symbolLocation":8,"imageIndex":61},{"imageOffset":28896,"symbol":"_pthread_cond_wait","symbolLocation":984,"imageIndex":62},{"imageOffset":2295220,"symbol":"QWaitConditionPrivate::wait(QDeadlineTimer)","symbolLocation":236,"imageIndex":12},{"imageOffset":2294880,"symbol":"QWaitCondition::wait(QMutex*, QDeadlineTimer)","symbolLocation":108,"imageIndex":12},{"imageOffset":2272456,"symbol":"QThreadPoolThread::run()","symbolLocation":932,"imageIndex":12},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1514450,"name":"Thread (pooled)","threadState":{"x":[{"value":260},{"value":0},{"value":5970432},{"value":0},{"value":0},{"value":160},{"value":29},{"value":999998000},{"value":6151990648},{"value":0},{"value":6144},{"value":26388279072770},{"value":26388279072770},{"value":6144},{"value":0},{"value":26388279072768},{"value":305},{"value":8830129424},{"value":0},{"value":105553174759552},{"value":105553174759616},{"value":6151991520},{"value":999998000},{"value":29},{"value":5970432},{"value":5970433},{"value":5970688},{"value":1},{"value":5195015488}],"flavor":"ARM_THREAD_STATE64","lr":{"value":6967951584},"cpsr":{"value":1610616832},"fp":{"value":6151990768},"sp":{"value":6151990624},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967694284},"far":{"value":0}},"frames":[{"imageOffset":17356,"symbol":"__psynch_cvwait","symbolLocation":8,"imageIndex":61},{"imageOffset":28896,"symbol":"_pthread_cond_wait","symbolLocation":984,"imageIndex":62},{"imageOffset":2295220,"symbol":"QWaitConditionPrivate::wait(QDeadlineTimer)","symbolLocation":236,"imageIndex":12},{"imageOffset":2294880,"symbol":"QWaitCondition::wait(QMutex*, QDeadlineTimer)","symbolLocation":108,"imageIndex":12},{"imageOffset":2272456,"symbol":"QThreadPoolThread::run()","symbolLocation":932,"imageIndex":12},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1514451,"name":"Thread (pooled)","threadState":{"x":[{"value":260},{"value":0},{"value":5994752},{"value":0},{"value":0},{"value":160},{"value":29},{"value":999999000},{"value":6152564088},{"value":0},{"value":6400},{"value":27487790700802},{"value":27487790700802},{"value":6400},{"value":0},{"value":27487790700800},{"value":305},{"value":8830129424},{"value":0},{"value":105553174759936},{"value":105553174760000},{"value":6152564960},{"value":999999000},{"value":29},{"value":5994752},{"value":5994753},{"value":5995008},{"value":1},{"value":5195015488}],"flavor":"ARM_THREAD_STATE64","lr":{"value":6967951584},"cpsr":{"value":1610616832},"fp":{"value":6152564208},"sp":{"value":6152564064},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967694284},"far":{"value":0}},"frames":[{"imageOffset":17356,"symbol":"__psynch_cvwait","symbolLocation":8,"imageIndex":61},{"imageOffset":28896,"symbol":"_pthread_cond_wait","symbolLocation":984,"imageIndex":62},{"imageOffset":2295220,"symbol":"QWaitConditionPrivate::wait(QDeadlineTimer)","symbolLocation":236,"imageIndex":12},{"imageOffset":2294880,"symbol":"QWaitCondition::wait(QMutex*, QDeadlineTimer)","symbolLocation":108,"imageIndex":12},{"imageOffset":2272456,"symbol":"QThreadPoolThread::run()","symbolLocation":932,"imageIndex":12},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1514454,"name":"Thread (pooled)","threadState":{"x":[{"value":260},{"value":0},{"value":5957376},{"value":0},{"value":0},{"value":160},{"value":29},{"value":999998000},{"value":6153137528},{"value":0},{"value":6912},{"value":29686813956866},{"value":29686813956866},{"value":6912},{"value":0},{"value":29686813956864},{"value":305},{"value":8830129424},{"value":0},{"value":105553241604480},{"value":105553241604544},{"value":6153138400},{"value":999998000},{"value":29},{"value":5957376},{"value":5957377},{"value":5957632},{"value":1},{"value":5195015488}],"flavor":"ARM_THREAD_STATE64","lr":{"value":6967951584},"cpsr":{"value":1610616832},"fp":{"value":6153137648},"sp":{"value":6153137504},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967694284},"far":{"value":0}},"frames":[{"imageOffset":17356,"symbol":"__psynch_cvwait","symbolLocation":8,"imageIndex":61},{"imageOffset":28896,"symbol":"_pthread_cond_wait","symbolLocation":984,"imageIndex":62},{"imageOffset":2295220,"symbol":"QWaitConditionPrivate::wait(QDeadlineTimer)","symbolLocation":236,"imageIndex":12},{"imageOffset":2294880,"symbol":"QWaitCondition::wait(QMutex*, QDeadlineTimer)","symbolLocation":108,"imageIndex":12},{"imageOffset":2272456,"symbol":"QThreadPoolThread::run()","symbolLocation":932,"imageIndex":12},{"imageOffset":2239744,"symbol":"QThreadPrivate::start(void*)","symbolLocation":364,"imageIndex":12},{"imageOffset":27660,"symbol":"_pthread_start","symbolLocation":136,"imageIndex":62},{"imageOffset":7040,"symbol":"thread_start","symbolLocation":8,"imageIndex":62}]},{"id":1514518,"frames":[{"imageOffset":7020,"symbol":"start_wqthread","symbolLocation":0,"imageIndex":62}],"threadState":{"x":[{"value":6154285056},{"value":175227},{"value":6153748480},{"value":0},{"value":409604},{"value":18446744073709551615},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":0},"cpsr":{"value":4096},"fp":{"value":0},"sp":{"value":6154285056},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967929708},"far":{"value":0}}},{"id":1514573,"frames":[{"imageOffset":7020,"symbol":"start_wqthread","symbolLocation":0,"imageIndex":62}],"threadState":{"x":[{"value":6155431936},{"value":253999},{"value":6154895360},{"value":0},{"value":409604},{"value":18446744073709551615},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":0},"cpsr":{"value":4096},"fp":{"value":0},"sp":{"value":6155431936},"esr":{"value":1442840704,"description":" Address size fault"},"pc":{"value":6967929708},"far":{"value":0}}},{"id":1514574,"frames":[{"imageOffset":7020,"symbol":"start_wqthread","symbolLocation":0,"imageIndex":62}],"threadState":{"x":[{"value":6154858496},{"value":0},{"value":6154321920},{"value":0},{"value":278532},{"value":18446744073709551615},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0},{"value":0}],"flavor":"ARM_THREAD_STATE64","lr":{"value":0},"cpsr":{"value":4096},"fp":{"value":0},"sp":{"value":6154858496},"esr":{"value":0,"description":" Address size fault"},"pc":{"value":6967929708},"far":{"value":0}}}],
  "usedImages" : [
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4329209856,
    "size" : 49152,
    "uuid" : "ff96842b-07be-35bd-a984-202ce8519030",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/MacOS\/qlcplus",
    "name" : "qlcplus"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4330881024,
    "size" : 425984,
    "uuid" : "8316d4e8-a6d4-32fd-8b57-fed552ae629d",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/Frameworks\/libqlcpluswebaccess.dylib",
    "name" : "libqlcpluswebaccess.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4335910912,
    "size" : 2998272,
    "uuid" : "4b88b4df-026d-32df-924c-28594207686e",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/Frameworks\/libqlcplusui.dylib",
    "name" : "libqlcplusui.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4333666304,
    "size" : 1376256,
    "uuid" : "0a2f005b-2324-3650-8fc2-46ecc2a1b3af",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/Frameworks\/libqlcplusengine.dylib",
    "name" : "libqlcplusengine.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4331503616,
    "size" : 557056,
    "uuid" : "407baed4-d9b0-3020-ac2c-c045b21ce470",
    "path" : "\/opt\/homebrew\/*\/libfftw3.3.dylib",
    "name" : "libfftw3.3.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4329455616,
    "CFBundleShortVersionString" : "6.9",
    "CFBundleIdentifier" : "org.qt-project.QtMultimediaWidgets",
    "size" : 32768,
    "uuid" : "bc2a69c6-2538-3bad-832c-f1fa54330dfc",
    "path" : "\/opt\/homebrew\/*\/QtMultimediaWidgets.framework\/Versions\/A\/QtMultimediaWidgets",
    "name" : "QtMultimediaWidgets",
    "CFBundleVersion" : "6.9.3"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4347543552,
    "CFBundleShortVersionString" : "6.9",
    "CFBundleIdentifier" : "org.qt-project.QtWidgets",
    "size" : 4636672,
    "uuid" : "bda58d3c-fd8f-3a83-995f-e3fb4bbe1caf",
    "path" : "\/opt\/homebrew\/*\/QtWidgets.framework\/Versions\/A\/QtWidgets",
    "name" : "QtWidgets",
    "CFBundleVersion" : "6.9.3"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4354768896,
    "CFBundleShortVersionString" : "6.9",
    "CFBundleIdentifier" : "org.qt-project.QtQml",
    "size" : 3899392,
    "uuid" : "62666d6c-4c42-39ca-a1f3-89e7072ad5a5",
    "path" : "\/opt\/homebrew\/*\/QtQml.framework\/Versions\/A\/QtQml",
    "name" : "QtQml",
    "CFBundleVersion" : "6.9.3"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4341923840,
    "CFBundleShortVersionString" : "6.9",
    "CFBundleIdentifier" : "org.qt-project.QtMultimedia",
    "size" : 835584,
    "uuid" : "91292b5a-4a41-3d49-a68f-e0622c017264",
    "path" : "\/opt\/homebrew\/*\/QtMultimedia.framework\/Versions\/A\/QtMultimedia",
    "name" : "QtMultimedia",
    "CFBundleVersion" : "6.9.3"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4369678336,
    "CFBundleShortVersionString" : "6.9",
    "CFBundleIdentifier" : "org.qt-project.QtGui",
    "size" : 5750784,
    "uuid" : "ec61ea9a-877a-3f04-b917-1f03807e1beb",
    "path" : "\/opt\/homebrew\/*\/QtGui.framework\/Versions\/A\/QtGui",
    "name" : "QtGui",
    "CFBundleVersion" : "6.9.3"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4329865216,
    "CFBundleShortVersionString" : "6.9",
    "CFBundleIdentifier" : "org.qt-project.QtWebSockets",
    "size" : 131072,
    "uuid" : "d7d2f3ec-e25f-348e-bc8a-a4009461dbd6",
    "path" : "\/opt\/homebrew\/*\/QtWebSockets.framework\/Versions\/A\/QtWebSockets",
    "name" : "QtWebSockets",
    "CFBundleVersion" : "6.9.3"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4361207808,
    "CFBundleShortVersionString" : "6.9",
    "CFBundleIdentifier" : "org.qt-project.QtNetwork",
    "size" : 1261568,
    "uuid" : "2b65f08f-d022-3c82-8916-db1c522e3cc9",
    "path" : "\/opt\/homebrew\/*\/QtNetwork.framework\/Versions\/A\/QtNetwork",
    "name" : "QtNetwork",
    "CFBundleVersion" : "6.9.3"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4385013760,
    "CFBundleShortVersionString" : "6.9",
    "CFBundleIdentifier" : "org.qt-project.QtCore",
    "size" : 4620288,
    "uuid" : "2790f9c7-9b75-3f4e-96b7-ed028bd42288",
    "path" : "\/opt\/homebrew\/*\/QtCore.framework\/Versions\/A\/QtCore",
    "name" : "QtCore",
    "CFBundleVersion" : "6.9.3"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4329570304,
    "size" : 49152,
    "uuid" : "cc77e640-3de5-3d9c-9565-c60ed8119746",
    "path" : "\/opt\/homebrew\/*\/libbrotlidec.1.1.0.dylib",
    "name" : "libbrotlidec.1.1.0.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4332879872,
    "size" : 573440,
    "uuid" : "afd03688-5fac-3b3d-96aa-9d88c034ff23",
    "path" : "\/opt\/homebrew\/*\/libzstd.1.5.7.dylib",
    "name" : "libzstd.1.5.7.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4343513088,
    "size" : 606208,
    "uuid" : "810d99f1-2504-3db7-9357-4bce9d47fc76",
    "path" : "\/opt\/homebrew\/*\/libssl.3.dylib",
    "name" : "libssl.3.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4378181632,
    "size" : 3407872,
    "uuid" : "efba4c7a-a8ab-3429-8076-3bf9f22bd40b",
    "path" : "\/opt\/homebrew\/*\/libcrypto.3.dylib",
    "name" : "libcrypto.3.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4391862272,
    "size" : 1785856,
    "uuid" : "6c0f5599-2c90-33c5-8a77-33c654b11c06",
    "path" : "\/opt\/homebrew\/*\/libicui18n.77.1.dylib",
    "name" : "libicui18n.77.1.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4363386880,
    "size" : 1277952,
    "uuid" : "c48fd9eb-06bb-3a7b-b4c7-a6e7da9f8b92",
    "path" : "\/opt\/homebrew\/*\/libicuuc.77.1.dylib",
    "name" : "libicuuc.77.1.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4427218944,
    "size" : 31899648,
    "uuid" : "e996519b-b7ad-3d2c-94e9-1ca42fc9cfb0",
    "path" : "\/opt\/homebrew\/*\/libicudata.77.1.dylib",
    "name" : "libicudata.77.1.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4344430592,
    "size" : 1032192,
    "uuid" : "a7cd8b10-d5ee-3abe-b185-81f2be4a1765",
    "path" : "\/opt\/homebrew\/*\/libglib-2.0.0.dylib",
    "name" : "libglib-2.0.0.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4329668608,
    "size" : 49152,
    "uuid" : "99e1194f-a3c6-30df-a9a2-5183f8ea3775",
    "path" : "\/opt\/homebrew\/*\/libdouble-conversion.3.3.0.dylib",
    "name" : "libdouble-conversion.3.3.0.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4330160128,
    "size" : 32768,
    "uuid" : "82f85bf6-93d0-3be3-9cc8-23cbf361e60f",
    "path" : "\/opt\/homebrew\/*\/libb2.1.dylib",
    "name" : "libb2.1.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4340334592,
    "size" : 442368,
    "uuid" : "f5623285-c810-3d39-8f96-8fc66d5b0601",
    "path" : "\/opt\/homebrew\/*\/libpcre2-16.0.dylib",
    "name" : "libpcre2-16.0.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4329783296,
    "size" : 16384,
    "uuid" : "058d0a42-f905-36cc-b767-c47265856de1",
    "path" : "\/opt\/homebrew\/*\/libgthread-2.0.0.dylib",
    "name" : "libgthread-2.0.0.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4332453888,
    "size" : 163840,
    "uuid" : "9414c162-5ccc-3b6d-8a0d-c7c743f7fec1",
    "path" : "\/opt\/homebrew\/*\/libintl.8.dylib",
    "name" : "libintl.8.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4345806848,
    "size" : 475136,
    "uuid" : "cb5b10c2-f523-342e-ac8d-aeb83b1612c6",
    "path" : "\/opt\/homebrew\/*\/libpcre2-8.0.dylib",
    "name" : "libpcre2-8.0.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4332208128,
    "size" : 131072,
    "uuid" : "cbb61532-d27c-331e-996e-843ed45f10e9",
    "path" : "\/opt\/homebrew\/*\/libbrotlicommon.1.1.0.dylib",
    "name" : "libbrotlicommon.1.1.0.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4346380288,
    "CFBundleShortVersionString" : "6.9",
    "CFBundleIdentifier" : "org.qt-project.QtDBus",
    "size" : 524288,
    "uuid" : "8b57c0ac-f526-303c-994c-b50c656b1db6",
    "path" : "\/opt\/homebrew\/*\/QtDBus.framework\/Versions\/A\/QtDBus",
    "name" : "QtDBus",
    "CFBundleVersion" : "6.9.3"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4340875264,
    "size" : 147456,
    "uuid" : "1d486868-3c8c-321e-9727-3a5ba5cd220e",
    "path" : "\/opt\/homebrew\/*\/libpng16.16.dylib",
    "name" : "libpng16.16.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4366565376,
    "size" : 802816,
    "uuid" : "00ed5fa9-de3f-3de0-b432-857eb401c764",
    "path" : "\/opt\/homebrew\/*\/libharfbuzz.0.dylib",
    "name" : "libharfbuzz.0.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4330258432,
    "size" : 49152,
    "uuid" : "bbb78130-97b1-3019-9bb2-db21119d3562",
    "path" : "\/opt\/homebrew\/*\/libmd4c.0.5.2.dylib",
    "name" : "libmd4c.0.5.2.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4365254656,
    "size" : 507904,
    "uuid" : "5642e153-03c7-3193-9276-871551a3a55e",
    "path" : "\/opt\/homebrew\/*\/libfreetype.6.dylib",
    "name" : "libfreetype.6.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4341415936,
    "size" : 196608,
    "uuid" : "fb8085dc-9746-335c-90d0-429dfe91dd60",
    "path" : "\/opt\/homebrew\/*\/libdbus-1.3.dylib",
    "name" : "libdbus-1.3.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4341088256,
    "size" : 81920,
    "uuid" : "dadd250f-e2ba-36f3-b960-ea391395d1e7",
    "path" : "\/opt\/homebrew\/*\/libgraphite2.3.2.1.dylib",
    "name" : "libgraphite2.3.2.1.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64e",
    "base" : 4366172160,
    "CFBundleShortVersionString" : "3.0",
    "CFBundleIdentifier" : "com.apple.security.csparser",
    "size" : 131072,
    "uuid" : "3a905673-ada9-3c57-992e-b83f555baa61",
    "path" : "\/System\/Library\/Frameworks\/Security.framework\/Versions\/A\/PlugIns\/csparser.bundle\/Contents\/MacOS\/csparser",
    "name" : "csparser",
    "CFBundleVersion" : "61439.140.12"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4395040768,
    "size" : 688128,
    "uuid" : "5fb652bc-d296-3802-8f4d-a9677091961d",
    "path" : "\/opt\/homebrew\/*\/libqcocoa.dylib",
    "name" : "libqcocoa.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64e",
    "base" : 4404658176,
    "size" : 49152,
    "uuid" : "a3faee04-0f8b-3428-9497-560c97eca6fb",
    "path" : "\/usr\/lib\/libobjc-trampolines.dylib",
    "name" : "libobjc-trampolines.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4505124864,
    "size" : 147456,
    "uuid" : "1559963e-26f9-3da8-9bba-9e3a97456f32",
    "path" : "\/opt\/homebrew\/*\/libqmacstyle.dylib",
    "name" : "libqmacstyle.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64e",
    "base" : 4540071936,
    "CFBundleShortVersionString" : "329.2",
    "CFBundleIdentifier" : "com.apple.AGXMetalG13X",
    "size" : 6914048,
    "uuid" : "6b497f3b-6583-398c-9d05-5f30a1c1bae5",
    "path" : "\/System\/Library\/Extensions\/AGXMetalG13X.bundle\/Contents\/MacOS\/AGXMetalG13X",
    "name" : "AGXMetalG13X",
    "CFBundleVersion" : "329.2"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4526833664,
    "size" : 180224,
    "uuid" : "0f8a39b7-2edf-31be-b4a3-e2e4fdf19af4",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/PlugIns\/libartnet.dylib",
    "name" : "libartnet.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4527996928,
    "size" : 262144,
    "uuid" : "127413c3-c54e-384d-854f-50c6e5fcf060",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/PlugIns\/libdmxusb.dylib",
    "name" : "libdmxusb.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4527538176,
    "size" : 49152,
    "uuid" : "3abe7ef1-b602-390b-a03f-6ddf5d65150c",
    "path" : "\/opt\/homebrew\/*\/libftdi1.2.5.0.dylib",
    "name" : "libftdi1.2.5.0.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4527816704,
    "size" : 98304,
    "uuid" : "b0d386d3-de68-3ac0-97aa-3ac07386ede9",
    "path" : "\/opt\/homebrew\/*\/libusb-1.0.0.dylib",
    "name" : "libusb-1.0.0.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4528668672,
    "CFBundleShortVersionString" : "6.9",
    "CFBundleIdentifier" : "org.qt-project.QtSerialPort",
    "size" : 81920,
    "uuid" : "093c8ddf-30db-3586-8f3e-3e2ef9737b21",
    "path" : "\/opt\/homebrew\/*\/QtSerialPort.framework\/Versions\/A\/QtSerialPort",
    "name" : "QtSerialPort",
    "CFBundleVersion" : "6.9.3"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4529143808,
    "size" : 131072,
    "uuid" : "bea0bccb-13ca-307f-8ea3-4b9de8d78f06",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/PlugIns\/libe131.dylib",
    "name" : "libe131.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4522819584,
    "size" : 81920,
    "uuid" : "19222aa1-fe97-39b5-962e-0f27812a5ea0",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/PlugIns\/libenttecwing.dylib",
    "name" : "libenttecwing.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4528881664,
    "size" : 98304,
    "uuid" : "39ff51e6-b47a-30dc-8ba8-d1db569837fa",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/PlugIns\/libhidplugin.dylib",
    "name" : "libhidplugin.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64e",
    "base" : 4527636480,
    "CFBundleShortVersionString" : "2.0.0",
    "CFBundleIdentifier" : "com.apple.iokit.IOHIDLib",
    "size" : 98304,
    "uuid" : "fdc90f28-4817-367a-b9e1-c0b89d530837",
    "path" : "\/System\/Library\/Extensions\/IOHIDFamily.kext\/Contents\/PlugIns\/IOHIDLib.plugin\/Contents\/MacOS\/IOHIDLib",
    "name" : "IOHIDLib",
    "CFBundleVersion" : "2.0.0"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4525604864,
    "size" : 49152,
    "uuid" : "5ec40bd1-182b-3928-83c7-0b28eb555e9d",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/PlugIns\/libloopback.dylib",
    "name" : "libloopback.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4530487296,
    "size" : 131072,
    "uuid" : "1668f041-c000-327f-90a2-0125e86d71a8",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/PlugIns\/libmidiplugin.dylib",
    "name" : "libmidiplugin.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4528455680,
    "size" : 65536,
    "uuid" : "07cea6d4-5111-37b2-b3b1-e83ae133c2c5",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/PlugIns\/libos2l.dylib",
    "name" : "libos2l.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4531945472,
    "size" : 131072,
    "uuid" : "3d58eb73-76b9-3905-ae1d-551648aed33c",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/PlugIns\/libosc.dylib",
    "name" : "libosc.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4529766400,
    "size" : 81920,
    "uuid" : "1af322fd-d2e4-3f77-8642-1d682595baab",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/PlugIns\/libpeperoni.dylib",
    "name" : "libpeperoni.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4529946624,
    "size" : 65536,
    "uuid" : "0de40b91-da1c-3ac4-b3d6-d33d4d74b877",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/PlugIns\/libudmx.dylib",
    "name" : "libudmx.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64",
    "base" : 4530749440,
    "size" : 49152,
    "uuid" : "7dede5e5-b1d5-3955-a231-172c68867ac9",
    "path" : "\/Users\/USER\/QLC+.app\/Contents\/PlugIns\/libvelleman.dylib",
    "name" : "libvelleman.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64e",
    "base" : 6968401920,
    "CFBundleShortVersionString" : "6.9",
    "CFBundleIdentifier" : "com.apple.CoreFoundation",
    "size" : 5500928,
    "uuid" : "8d45baee-6cc0-3b89-93fd-ea1c8e15c6d7",
    "path" : "\/System\/Library\/Frameworks\/CoreFoundation.framework\/Versions\/A\/CoreFoundation",
    "name" : "CoreFoundation",
    "CFBundleVersion" : "3603"
  },
  {
    "source" : "P",
    "arch" : "arm64e",
    "base" : 7163805696,
    "CFBundleShortVersionString" : "2.1.1",
    "CFBundleIdentifier" : "com.apple.HIToolbox",
    "size" : 3174368,
    "uuid" : "1a037942-11e0-3fc8-aad2-20b11e7ae1a4",
    "path" : "\/System\/Library\/Frameworks\/Carbon.framework\/Versions\/A\/Frameworks\/HIToolbox.framework\/Versions\/A\/HIToolbox",
    "name" : "HIToolbox"
  },
  {
    "source" : "P",
    "arch" : "arm64e",
    "base" : 7034875904,
    "CFBundleShortVersionString" : "6.9",
    "CFBundleIdentifier" : "com.apple.AppKit",
    "size" : 21564992,
    "uuid" : "860c164c-d04c-30ff-8c6f-e672b74caf11",
    "path" : "\/System\/Library\/Frameworks\/AppKit.framework\/Versions\/C\/AppKit",
    "name" : "AppKit",
    "CFBundleVersion" : "2575.70.52"
  },
  {
    "source" : "P",
    "arch" : "arm64e",
    "base" : 6964117504,
    "size" : 636280,
    "uuid" : "3247e185-ced2-36ff-9e29-47a77c23e004",
    "path" : "\/usr\/lib\/dyld",
    "name" : "dyld"
  },
  {
    "size" : 0,
    "source" : "A",
    "base" : 0,
    "uuid" : "00000000-0000-0000-0000-000000000000"
  },
  {
    "source" : "P",
    "arch" : "arm64e",
    "base" : 6967676928,
    "size" : 243284,
    "uuid" : "6e4a96ad-04b8-3e8a-b91d-087e62306246",
    "path" : "\/usr\/lib\/system\/libsystem_kernel.dylib",
    "name" : "libsystem_kernel.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64e",
    "base" : 6967922688,
    "size" : 51784,
    "uuid" : "d6494ba9-171e-39fc-b1aa-28ecf87975d1",
    "path" : "\/usr\/lib\/system\/libsystem_pthread.dylib",
    "name" : "libsystem_pthread.dylib"
  },
  {
    "source" : "P",
    "arch" : "arm64e",
    "base" : 8589369344,
    "CFBundleShortVersionString" : "1.0",
    "CFBundleIdentifier" : "com.apple.audio.AVFAudio",
    "size" : 1266176,
    "uuid" : "bf6d4128-7675-3fa9-9519-cf4dae7d7bab",
    "path" : "\/System\/Library\/Frameworks\/AVFAudio.framework\/Versions\/A\/AVFAudio",
    "name" : "AVFAudio"
  },
  {
    "source" : "P",
    "arch" : "arm64e",
    "base" : 7444332544,
    "CFBundleShortVersionString" : "2.0",
    "CFBundleIdentifier" : "com.apple.audio.midi.CoreMIDI",
    "size" : 765216,
    "uuid" : "504d9a4a-f0a7-348f-a7bc-13fd26b48d99",
    "path" : "\/System\/Library\/Frameworks\/CoreMIDI.framework\/Versions\/A\/CoreMIDI",
    "name" : "CoreMIDI",
    "CFBundleVersion" : "88"
  },
  {
    "source" : "P",
    "arch" : "arm64e",
    "base" : 7159255040,
    "CFBundleShortVersionString" : "1.0",
    "CFBundleIdentifier" : "com.apple.audio.caulk",
    "size" : 163296,
    "uuid" : "42085f32-42e2-3f11-b0b4-0343137b5f72",
    "path" : "\/System\/Library\/PrivateFrameworks\/caulk.framework\/Versions\/A\/caulk",
    "name" : "caulk"
  },
  {
    "source" : "P",
    "arch" : "arm64e",
    "base" : 6966444032,
    "size" : 528964,
    "uuid" : "dfea8794-80ce-37c3-8f6a-108aa1d0b1b0",
    "path" : "\/usr\/lib\/system\/libsystem_c.dylib",
    "name" : "libsystem_c.dylib"
  }
],
  "sharedCache" : {
  "base" : 6963281920,
  "size" : 5040898048,
  "uuid" : "4c1223e5-cace-3982-a003-6110a7a8a25c"
},
  "vmSummary" : "ReadOnly portion of Libraries: Total=1.7G resident=0K(0%) swapped_out_or_unallocated=1.7G(100%)\nWritable regions: Total=2.1G written=1733K(0%) resident=1285K(0%) swapped_out=448K(0%) unallocated=2.1G(100%)\n\n                                VIRTUAL   REGION \nREGION TYPE                        SIZE    COUNT (non-coalesced) \n===========                     =======  ======= \nAccelerate framework               128K        1 \nActivity Tracing                   256K        1 \nCG image                          2240K       23 \nColorSync                          656K       35 \nCoreAnimation                     1968K      104 \nCoreGraphics                        64K        4 \nCoreUI image data                 5360K       42 \nFoundation                          48K        2 \nImage IO                            32K        2 \nJS VM Gigacage                    4096K        1 \nJS VM Isolated Heap               6464K        5 \nKernel Alloc Once                   32K        1 \nMALLOC                             2.1G      125 \nMALLOC guard page                  288K       18 \nSTACK GUARD                       56.5M       29 \nStack                             22.9M       30 \nVM_ALLOCATE                        512K       23 \n__AUTH                            5446K      688 \n__AUTH_CONST                      76.4M      928 \n__CTF                               824        1 \n__DATA                            26.3M      959 \n__DATA_CONST                      28.9M      990 \n__DATA_DIRTY                      2763K      336 \n__FONT_DATA                        2352        1 \n__INFO_FILTER                         8        1 \n__LINKEDIT                       637.9M       57 \n__OBJC_RO                         61.4M        1 \n__OBJC_RW                         2396K        1 \n__TEXT                             1.1G     1012 \n__TPRO_CONST                       128K        2 \ndyld private memory                128K        1 \nmapped file                      592.6M       73 \npage table in kernel              1285K        1 \nshared memory                     1392K       14 \n===========                     =======  ======= \nTOTAL                              4.7G     5512 \n",
  "legacyInfo" : {
  "threadTriggered" : {
    "queue" : "com.apple.main-thread"
  }
},
  "logWritingSignature" : "f0814be710542b6cfac700107e7f149d508d9775",
  "trialInfo" : {
  "rollouts" : [
    {
      "rolloutId" : "6434420a89ec2e0a7a38bf5a",
      "factorPackIds" : {

      },
      "deploymentId" : 240000011
    },
    {
      "rolloutId" : "66d35d7fe4d6bf7664f40ddf",
      "factorPackIds" : {
        "BLACKPEARL_SPARROW" : "67c7824a1baae429bb41b897"
      },
      "deploymentId" : 240000069
    }
  ],
  "experiments" : [

  ]
}
}

