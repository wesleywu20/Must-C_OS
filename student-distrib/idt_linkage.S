# idt_linkage.S - assembly linkage for handlers for IDT

#define ASM 1

#define INTR_LINK(name, func) \
  .globl name                ;\
  name:                      ;\
    pushal                   ;\
    pushfl                   ;\
    call func                ;\
    popfl                    ;\
    popal                    ;\
    iret

INTR_LINK(keyboard_interrupt_handler, _keyboard_interrupt_handler)
INTR_LINK(rtc_interrupt_handler, _rtc_interrupt_handler)
INTR_LINK(timer_handler, _timer_handler)
