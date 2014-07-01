An Experimental Comparison of Concurrent Data Structures 
===
Mark Gibson - 10308693
===

##Structure

Inside the Data Structures directory I have seperated my work into the three sub directories, with each representing a module of my project.

For example, heading into the Ring_Buffer directory, displays the source code I use in implementing in project and in addition, the data I have collected.

All Excel Speadsheets appended with the term '_old' are outdated data that I collected and was not used in the final version of my project. This could have been due to several reasons, say for example, that I discovered an error in my data collection methods or I devised a more efficient implementation of an algorithm.

##Ring Buffer

So, in the Ring_Buffer directory, ring.cpp and spsc.cpp are the two source files that contain the code for the multi-producer multi-consumer locked implementations of the ring buffer and the single-producer single-consumer lockless implementation of the ring buffer respectively.

The relevant data for both ring.cpp and spsc.cpp is contained in Ring_ Buffer_ Test_ Data.xlsx.

##Linked List

Inside the linked list directory, three more directories are found along with several. Each of these leads to their respective variations of the linked list. Going into "Doubly Linked Buffer" presents much the same view as with the ring buffer. mpmc_ locked.cpp contains the locked implementations of the doubly linked buffer, with mpmc_ lockless containing the lockless implementation. 

median.h is a header I file to define the median calculation used as part of the data collection process. 

doubly _ linked_buffer is a script file written in bash that compiles each implementation and runs it, using the tool perf to gather hardware performance data on it.

The relevant data is stored in MPMC_Buffer _Test _ Data.xlsx.

What has just been described is the same for the other two variations of the linked list, just with file names changed etc.

##Hash Table

The hash table is slightly different to the linked list, there are three source files, hash_ locked, hash_ locked _ per _ bucket and hash_lockless which contain their respective implementations of the hash table.

Each implementation also has its own script to compile and run it, hscript_locked etc.

Finally all data is stored in hash_table_data.xlsx, with all other Excel files containing outdated data.