# embos
embOS is a priority-controlled real time operating system, designed to be used as foundation for the development of embedded real-time applications. It is a zero interrupt latency, high-performance RTOS that has been optimized for minimum memory consumption in both RAM and ROM, as well as high speed and versatility.

Throughout the development process of embOS, the limited resources of microcontrollers have always been kept in mind. The internal structure of embOS has been optimized in a variety of applications with different customers, to fit the needs of different industries.

embOS is fully source-compatible on different platforms (8/16/32 bits), making it easy to port applications to different CPUs. Its highly modular structure ensures that only the functions needed are linked, keeping the ROM size very small. Tasks can easily be created and safely communicate with each other using the complete panoply of communication mechanisms such as semaphores, mailboxes, and events. Interrupt Service Routines (ISRs) can also take advantage of these communication mechanisms. 
