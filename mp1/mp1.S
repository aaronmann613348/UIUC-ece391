
.data					# section declaration

        # Useful offset constants for accessing members of a 
        # struct mp1_blink_struct structure
        LOCATION   = 0    
        ON_CHAR    = 2
        OFF_CHAR   = 3 
        ON_LENGTH  = 4
        OFF_LENGTH = 6
        COUNTDOWN  = 8
        STATUS     = 10
        NEXT       = 12

        STRUCT_SIZE = 16

# Pointer to head of list (initialized to NULL)
mp1_list_head:
        .long   0


function_table:                 #required for the "dispatcher"

        .long   mp1_ioctl_add      
        .long   mp1_ioctl_remove
        .long   mp1_ioctl_find      
        .long   mp1_ioctl_sync
              


.text					# section declaration

# Export the function symbol names

.global mp1_rtc_tasklet
.global mp1_ioctl

# void mp1_poke(void);
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX
mp1_poke:
        
	movl    vmem_base_addr(,1),%edx
	movb    %cl,(%edx,%eax,1)
	ret

mp1_rtc_tasklet:
	#start with same bull shit as add
        pushl     %ebp                          #save old frame Pointer
        movl      %esp, %ebp                    #point to new frame
        pushl      %esi         #source index
        pushl      %edi         #destination index
        pushl      %ebx         #preserve base
        movl mp1_list_head, %ebx #put the head of the list in ebx
        cmpl $0, %ebx
        je error


        #lets ride
first_loop:
        #check base condition first
        cmpl $0, %ebx   #check if the list is emtpy
        je      end     #leave the program if the list is empty!
        #now we loop
        decw COUNTDOWN(%ebx) #first decrement the countdown
        cmpw $0, COUNTDOWN(%ebx) #check the countdown against zero
        jg     loop_again #go to the next thing on the list and then loop again

        #becasue of loop again, this thing will loop through until it gets a zero countdown.
        #so, if it finally doesn't loop again we have countdown = 0
        #check the status on this bitch
        movzwl LOCATION(%ebx), %eax # put location in eax
        shll $1, %eax

 check_status:
        cmpw $0, STATUS(%ebx) #compare status bit to zero
        je off
        jne on

on:
        xorl %eax, %eax
        xorl %ecx, %ecx

        movb OFF_CHAR(%ebx), %cl #want OPPOSITE!
        movzwl LOCATION(%ebx), %eax # put location in eax
        shll $1, %eax
        call mp1_poke
        movw OFF_LENGTH(%ebx), %dx
        movw %dx, COUNTDOWN(%ebx)
        movw $0x0, STATUS(%ebx) #put hex 0 into the STATUS spot
        jmp loop_again
       



off:
        xorl %eax, %eax
        xorl %ecx, %ecx
        
        movb ON_CHAR(%ebx), %cl #want OPPOSITE!
         movzwl LOCATION(%ebx), %eax # put location in eax
        shll $1, %eax
        call mp1_poke
         movw ON_LENGTH(%ebx), %dx
         movw %dx, COUNTDOWN(%ebx)
        movw $0x1, STATUS(%ebx) #put hex 1 into the STATUS spot
        
loop_again:
        movl NEXT(%ebx), %ebx #move to the next element on the list
        jmp first_loop #then go back to the first loop
       
      

end:
        popl %ebx
        popl %edi
        popl %esi
        popl %ebp

        ret
error:
        movl $-1, %eax



mp1_ioctl:                                      #dispatcher    
                                
                                                #writing this first
                                                #gonna need a function table (jump table)
                                                #put shit on stack

        movl    8(%esp), %eax                   #move stack pointer into our general purpose Register


        cmpl    $3, %eax                       #compare to 3 
        jg      invalid                              #greater, go to invalid

        cmpl    $0, %eax                       #compare it to 0
        jl      invalid                         #less than zero, go to invalid
        


        jmp     *function_table(, %eax, 4)       #jump to function table  

	
invalid: #invalid subroutine return -1
        movl $-1, %eax                          #put -1 in eax
        ret


mp1_ioctl_add:
	
        pushl     %ebp                          #save old frame Pointer
        movl      %esp, %ebp                    #point to new frame
        #got all that form page 24 of course notes
        #I'm gonna also save the other Registers

        pushl      %esi         #source index
        pushl      %edi         #destination index
        pushl      %ebx         #preserve base

        movl 8(%ebp), %eax
        cmpl $0, %eax
        je fail
        pushl           $STRUCT_SIZE    #push the size on
        call            mp1_malloc      #allocate the memory
        addl $4, %esp 
        cmpl $0, %eax   #test if the malloc worked
        je fail  # go to fail helper function if the malloc returned a null

        #now make a copy
        movl %eax, %ebx #put a pointer to the structure in ebx by putting ebp-stuct size in it
        cmpl $0, %eax
        je fail


        pushl %eax
        #before we can call copy, we need to push on 3 things!
        pushl $STRUCT_SIZE #put size on stack
        pushl 8(%ebp) #copy over appropriate pointers for the struct
        pushl %ebx #copy over appropriate pointers for the struct
        call mp1_copy_from_user #now call copy from user
        addl $12, %esp
        #check for failure
        cmpl $0, %eax #anything other than zero is a fail
        jne copy_fail #anything other than zero is a fail... /=0 is a fail
        #need to check location thing now (don't totally get this)

        cmpw $80*25-1, LOCATION(%ebx) #use w becasue location is unsigned short
        ja fail  #send it to fail if the location is off.. that is, if the difference is greater than zero

        #we're good to go lets set the STATUS
        #using mov notes from page 20 of notes

        movw ON_LENGTH(%ebx), %dx # DX<-M[EDX+ON_LENGTH]
        movw %dx, COUNTDOWN(%ebx) # put on length in countdown!!!
         movw $0x1, STATUS(%ebx) #put hex 1 into the STATUS spot
       

        #changind d to b from here down
        popl %edx
        #now add this thing to the list head
        movl    mp1_list_head, %eax #hold head pointer
        movl    %eax, NEXT(%edx) #set next pointer
        movl    %edx, mp1_list_head #set new head

        #finally make poke call

                                #appropriate register parameters?

        movzwl LOCATION(%edx), %eax # put the correct offset in eax
        shll $1, %eax
        movb ON_CHAR(%edx), %cl # use movb becasue cl is low 8 bits.. put ascii to write there
        call mp1_poke           #call this thing
        jmp success             #done

copy_fail:
        call mp1_free   #this fail also needs to have a clear for the malloc
        movl $-1, %eax #put -1 in for the return it it failed
        
        jmp leave_helper #leave helper pops everythig stored off 

fail: 
        movl $-1, %eax #put -1 in for the return it it failed
        jmp leave_helper #leave helper pops everythig stored off 
leave_helper:
        popl %ebx       #pop off stored stuff 
        popl %edi       #pop off stored stuff 
        popl %esi      #pop off stored stuff   
        leave 
        ret    
success:
        movl $0, %eax #return 0
        jmp leave_helper


        
mp1_ioctl_remove:
	    pushl     %ebp                          #save old frame Pointer
        movl      %esp, %ebp                    #point to new frame
        pushl      %esi         #source index
        pushl      %edi         #destination index
        pushl      %ebx         #preserve base
        pushl 8(%ebp)
        call find_helper
        addl $4, %esp 
        cmpl $0, %eax   
        je remove_failed

        movl mp1_list_head, %edx
remove_loop:
        cmpl $0, %edx
        je remove_failed
        cmpl %eax, (%edx)
        je found
        movl NEXT(%edx), %edx
        jmp remove_loop


found:
    movl NEXT(%eax), %ecx
    movl %ecx, (%edx)


free_allocated:
    pushl %eax
    call mp1_free
    addl $4, %esp
    jmp remove_worked

remove_failed:
    movl $-1, %eax
    jmp leave_remove

remove_worked:
    movl $0, %eax

leave_remove:
    popl %ebx
    popl %edi
    popl %esi
    leave
    ret


        
mp1_ioctl_find:
        pushl     %ebp                          #save old frame Pointer
        movl      %esp, %ebp                    #point to new frame
        pushl      %esi         #source index
        pushl      %edi         #destination index
        pushl      %ebx         #preserve base

        
        movl 8(%ebp), %ebx #put a pointer to the structure in ebx by putting ebp-stuct size in it
        cmpl $0, %ebx
        je find_failed

        pushl %eax
        call find_helper
        addl $4, %esp
        cmpl $0, %eax
        je find_failed    

        pushl $STRUCT_SIZE 
        pushl %eax 
        pushl 8(%ebp) #copy over appropriate pointers for the struct
        call mp1_copy_to_user #now call copy from user
        addl $12, %esp
        cmpl $0, %eax
        je find_worked



find_worked:
        movl $0, %eax

find_failed:
        movl $-1, %eax
        jmp leave_find
leave_find:
        popl %ebx
        popl %edi
        popl %esi
        leave
        ret


find_helper:
        pushl     %ebp
        movl      %esp, %ebp 
        pushl %ebx #save that shit!
        movw 8(%ebp), %ax
        movl mp1_list_head, %ebx #get a  pointer


mainloop:
        cmpl $0, %ebx #check for null
        je find_helper_failed

        cmpw %ax, LOCATION(%ebx)
        je find_helper_worked   
        
        movl %ebx, %ecx #save previous pointer
        movl NEXT(%ebx), %ebx #move to next on list
        jmp mainloop #keep looping

find_helper_worked:
        movl %ebx, %eax

find_helper_failed:
        xorl %eax, %eax #clear
        jmp leave_find_helper

leave_find_helper:
        popl %ebx #restore
        leave
        ret










mp1_ioctl_sync:

        pushl     %ebp                          #save old frame Pointer
        movl      %esp, %ebp                    #point to new frame
        pushl      %esi         #source index
        pushl      %edi         #destination index
        pushl      %ebx         #preserve base

        #lets get started
        movl 8(%ebp), %eax
        movw %ax, %dx #split argument
        pushl %dx
        call find_helper
        addl $4, %esp
        cmpl $0, %eax
        je sync_failed
        movl %eax, %esi

        movl 8(%ebp), %eax
        shrl $16, %eax
        pushl %ax
        call find_helper
        addl $4, %esp
        cmpl $0, %eax
        je sync_failed
        movl %eax, %edi

        andl $0, %eax

        movw ON_LENGTH(%edi), %ax
        movw %ax, ON_LENGTH(%esi)
        movw OFF_LENGTH(%edi), %ax
        movw %ax, OFF_LENGTH(%esi)
        movw COUNTDOWN(%edi), %ax
        movw %ax, COUNTDOWN(%esi)
        movw STATUS(%edi), %ax
        movw %ax, STATUS(%esi)


check_status_sync:    
        movw STATUS(%edi), %cx
        cmpw $0, %cx
        jne sync_on
        jmp sync_off

sync_on:

        movb OFF_CHAR(%edi), %cl #want OPPOSITE!
        xorl %eax, %eax
        movzwl LOCATION(%edi), %eax # put location in eax
        shll $1, %eax
        call mp1_poke
        movw $0, STATUS(%edi)
        jmp sync_worked
       
sync_off:
      
        movb ON_CHAR(%edi), %cl #want OPPOSITE!
        xorl %eax, %eax
        movzwl LOCATION(%edi), %eax # put location in eax
        shll $1, %eax
        call mp1_poke
        movw $1, STATUS(%edi)
        jmp sync_worked
        
        
sync_failed:
        movl $-1, %eax
        jmp done_with_sync

sync_worked:
        movl $0, %eax

done_with_sync:
        popl %ebx
        popl %edi
        popl %esi

        leave
        ret



.end
