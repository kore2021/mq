# MultiQueue
## Copyright
Evgeny Korostelev, 2021


## Build Steps
Yoy need __Build Tools for Visual Studio 2015__. For example, run _Developer Command Prompt For VS2015_ and type this build command
```
cl /EHsc /W4 /WX main.cpp MultiQueue\Queue.cpp MultiQueue\QueueConsumer.cpp MultiQueue\QueueEventAdapter.cpp MultiQueue\Thread.cpp MultiQueue\ThreadPool.cpp
```
