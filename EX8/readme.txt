For the program heaploop:
Instruction pages: 21
Data pages: 194

In the instruction pages,
Page 0x7fff5fc17 is accessed the most probably because we are esstentially
doing a lot of data manipulation in the loop. This page most likely had mv
and add insturction in it.

In the datapages, the page 0x10801 is accessed the most. I think that it
may be that the program had the loop condition in this page where it need
to be get out, added and later checked again


For the matmul program:
Instruction pages: 27
Data pages: 462

In the instruction pages, the page 0x100000 is accessed the most. One possible
explanation would be that the program must go though every element of the matrix
and read the data, multiply and write it back which causes a huge access to the
arithmetic insturctions.

In the datapages, the page 0x108001 is accessed the most, this is probably the temp
value where the multiplication stores the result and later got write to the new location

