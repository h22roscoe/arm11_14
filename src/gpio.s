ldr r0,=0x20200000
mov r1,#1
lsl r1,#18
str r1,[r0,#4]
mov r5,#1
lsl r5,#16

on_off_loop:

ldr r3,=0xFFA4E
ldr r4,=0xFFA4E

str r5,[r0,#28]
on_loop:
sub r3,r3,#1
cmp r3,#0
bne on_loop

str r5,[r0,#40]
off_loop:
sub r4,r4,#1
cmp r4,#0
bne off_loop

b on_off_loop
