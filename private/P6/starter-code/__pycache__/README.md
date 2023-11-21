Personal Information:
Name: Hanlin Wang
CS Login: hanlin
Wisc ID: 9086237956
Wisc Netid: hwang2377
Email: hwang2377@wisc.edu

Name: Minchao Huang
CS Lohin: minchao
Wisc ID: 9084711127
Wisc Netid: mhuang249
Email: mhuang249@wisc.edu




Implementation
1. queue implementation in safequeue.c and safequeue.h
2. listener and worker thread created by main
3. listener thread will call serve_forever
4. serve_forever handles queue empty, queue full, GetJob, add_work
5. worker thread will cal serve_request