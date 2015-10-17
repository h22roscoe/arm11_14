ldr r12, =0x20200000
// IO access location
ldr r0, =0x1249000
str r0, [r12, #4]
ldr r0, =0x1000
str r0, [r12]
mov r0, #0
str r0, [r12, #8]
// Setting GPIO pins


mov r0, #400
// Max array length
mov r1, #23
// Seed value
ldr r2, =252133
// Add value
ldr r3, =3433413
// Mul value
mov r4, #257
// Div value
mov r5, #0
// Current round number
mov r7, #0x1000
// Start of array location


fill_array:
// Loading array with random numbers
mov r6, #0
// Used as iterator
fill_loop:
cmp r6, r0
bge game_loop
mov r8, r1
b rand_gen
end_rand_gen:
cmp r8, r1
beq rand_gen
str r1, [r7, r6]
add r6, r6, #4
b fill_loop


rand_gen:
ldr r11, =0x20003000
// mla r1, r1, r3, r2
ldr r1, [r11, #4]
and r1, r1, #15
b div
end_div:
mov r1, r10
b mod_4
end_mod_4:
// add r2, r2, #13
// sub r3, r3, #7
// add r4, r4, #3
b end_rand_gen


div:
// r10 = r1 `div` r4
mov r10, #0
loop_div:
cmp r1, r4
blt end_div
sub r1, r1, r4
add r10, r10, #1
b loop_div


mod_4:
// r1 = r1 % 4
sub_4_loop:
cmp r1, #4
blt end_mod_4
sub r1, r1, #4
b sub_4_loop


game_loop:
//mov r5, #400
add r5, r5, #4
b display_leds
end_display:
b check_pattern
end_check_pattern:
b game_loop


display_leds:
// Display the LEDs upto round number
mov r6, #0
display_loop:
cmp r6, r5
bge end_display
ldr r8, [r7, r6]
b light_led_chosen
end_light_led_chosen:
add r6. r6, #4
b display_loop


light_led_chosen:
// Light the LED at r8.
mov r11, #1
lsl r11, #14
cmp r8, #2
blt skip_shift_one
lsl r11, #1
skip_shift_one:
lsl r11, r8
str r11, [r12, #28]
// On delay
ldr r1, =0x800000
on_loop:
sub r1, r1, #1
cmp r1, #0
bge on_loop
mov r11, #1
lsl r11, #14
cmp r8, #2
blt skip_shift_one_2
lsl r11, #1
skip_shift_one_2:
lsl r11, r8
str r11, [r12, #40]
// Off delay
ldr r1, =0x100000
off_loop:
sub r1, r1, #1
cmp r1, #0
bge off_loop
b end_light_led_chosen


check_pattern:
mov r6, #0
mov r10, #0
// r10 is the previous value
check_loop:
cmp r6, r5
bge end_check_pattern
load_r9:
ldr r9, [r12, #52]
mov r9, r9, lsr #22
and r9, r9, #15
cmp r9, r10
beq load_r9
cmp r9, #0
beq load_r9
// We know that r9 is different to the previous value and not zero.
mov r10, r9
mov r3, #0
log_2_loop:
mov r9, r9, lsr #1
cmp r9, #0
beq end_log_2
add r3, r3, #1
b log_2_loop
end_log_2:
ldr r8, [r7, r6]
add r6, r6, #4
cmp r8, r3
beq check_loop
b end


end:
mov r10, #0x10
str r10, [r12, #28]
b end
