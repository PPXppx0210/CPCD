.data
.text
lui $sp,0x1002
j main
demo:
sw $ra 4($sp)
lw $s7 8($sp)
addi $s6 $zero 2
add $s7 $s7 $s6
addi $s5 $zero 2
mul $s4 $s7 $s5
sw $s4 12($sp)
add $v0 $zero $s4
lw $ra 4($sp)
jr $ra
main:
addi $s7 $zero 3
addi $s6 $zero 4
addi $s5 $zero 2
sw $s7 8($sp)
sw $s6 12($sp)
sw $s5 16($sp)
sw $s5 28($sp)
sw $sp 20($sp)
addi $sp $sp 20
jal demo
lw $sp 0($sp)
lw $s5 16($sp)
lw $s6 12($sp)
lw $s7 8($sp)
sw $s7 8($sp)
sw $s6 12($sp)
sw $s7 24($sp)
sw $s6 28($sp)
sw $v0 32($sp)
sw $sp 16($sp)
addi $sp $sp 16
jal program
lw $sp 0($sp)
lw $s6 12($sp)
lw $s7 8($sp)
j end
program:
sw $ra 4($sp)
addi $s7 $zero 0
lw $s6 12($sp)
lw $s5 16($sp)
add $s4 $s6 $s5
sw $s7 20($sp)
lw $s3 8($sp)
bgt $s3 $s4 Label2
Label1:
J Label3
Label2:
lw $s7 12($sp)
lw $s6 16($sp)
mul $s7 $s7 $s6
addi $s6 $zero 1
add $s7 $s7 $s6
lw $s5 8($sp)
add $s5 $s5 $s7
sw $s5 24($sp)
J Label4
Label3:
lw $s7 8($sp)
sw $s7 24($sp)
Label4:
lw $s7 20($sp)
addi $s6 $zero 100
ble $s7 $s6 Label6
Label5:
J Label7
Label6:
lw $s7 24($sp)
addi $s6 $zero 2
mul $s5 $s7 $s6
addi $s4 $zero 2
mul $s7 $s7 $s4
sw $s5 20($sp)
sw $s7 24($sp)
J Label4
Label7:
lw $v0 20($sp)
lw $ra 4($sp)
jr $ra
end:
