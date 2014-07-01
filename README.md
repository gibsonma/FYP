An Experimental Comparison of Concurrent Data Structures 
===
Mark Gibson
===

##My Work

The purpose of this project is to determine and compare the differences between concurrent data structure implementations and investigate whether the performance of these implementations is maintained across different architectures.

To do this I have implemented three concurrent data structures, a ring buffer, linked list and a hash table. Each data structure has several variations with regards to how they operate, such as the utilisation and placement of different pointers.

I have implemented each data structure with a mixture of locked and lockless algorithms. Among the locks used are a simple pthread mutex lock [Barney et al, 2013], a compare-and-swap lock and a ticket lock [Herlihy & Shavit, 2008]. For the lockless algorithms I use the C++ 11 atomic library [Atomic Operations Library, 2013] which contains the necessary atomic operations to implement lockless algorithms.

I have gathered data from these three data structures using varying thread counts and other variables, such as list or table size. The data structures are compared on a total of three different architectures to determine whether the performance of the algorithms is robust across architectures or not.

##Project Structure

Inside the Data Structures directory I have seperated my work into the three sub directories, with each representing a module of my project.

For example, heading into the Ring_Buffer directory, displays the source code I use in implementing in project and in addition, the data I have collected.

All Excel Speadsheets appended with the term '_old' are outdated data that I collected and was not used in the final version of my project. This could have been due to several reasons, say for example, that I discovered an error in my data collection methods or I devised a more efficient implementation of an algorithm.
