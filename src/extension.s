ldr r0, =0x20200000
ldr r4, =0x10000
mov r5, #0
ldr r6, =2178013
ldr r7, =253011
ldr r8, =257
ldr r12, =13
mov r1, #1
lsl r1, #24
mov r2, #1
lsl r2, #12
orr r1, r1, r2
lsl r2, #3
orr r1, r1, r2
lsl r2, #6
orr r1, r1, r2
str r1, [r0, #4]
mov r2, #1
lsl r2, #6
str r2, [r0]
// setting gpio pins to output.


get_input:
ldr r1, [r0, #52]
mov r12, r1, lsr #22
and r12, r12, #15
// b wait_for_input
// got_input:
b log_2
end_log_2:
add r5, r5, #4
str r10, [r4, r5]
cmp r5, #16
bne get_input
b light_leds


// wait_for_input:
// ldr r1, [r0, #52]
// mov r1, r1, lsr #22
// and r1, r1, #15
// cmp r1, r12
// beq wait_for_input
// cmp r1, #0
// beq wait_for_input
// mov r12, r1
// b got_input


log_2:
mov r10, #0
log_2_loop:
mov r12, r12, lsr #1
cmp r12, #0
beq end_log_2
add r10, r10, #1
b log_2_loop


print_leds:
mov r5, #0
led_loop:
add r5, r5, #4
ldr r12, [r4, r5]
b light_led_chosen
end_call_light_led:
cmp r5, #16
bne led_loop
b end


light_led_chosen:
// turns on the gpio in r12
mov r2, #1
lsl r2, #14
cmp r12, #2
blt shift_by_val
lsl r2, #1
shift_by_val:
lsl r2, r12
str r2, [r0, #28]
b delay


off_led_chosen:
str r2, [r0, #40]
b small_delay
end_small_delay:
b end_call_light_led


small_delay:
ldr r3, =0xF0000
small_loop:
sub r3, r3, #1
cmp r3, #0
bne small_loop
b end_small_delay


delay:
ldr r3, =0x800000
loop:
sub r3, r3, #1
cmp r3,#0
bne loop
b off_led_chosen


end:
mov r12, #1
lsl r12, #2
str r12, [r0, #28]
b end
