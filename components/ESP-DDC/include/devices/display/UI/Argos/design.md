
3 top layer pages, each represent one tab (INFO/NETWORK/ABOUT)

each top layer page has a stack, in which subpage is push and pop

 ___________________________back____________________________
 |                                                         |
INFO ----> switch to ----> NETWORK ----> switch to ----> ABOUT   --- top pages
 |                            |                            |
 sub page A                sub pageA                   sub_pageA --- stack depth 1
 |                            |                            |
 sub page B                sub pageB                   sub_pageB --- stack depth 2

 In any top page, or each's sub page,
 it should be able to read system state, or change it,
 and it should be able to send message from UI task to other task.