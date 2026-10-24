ENTRY(_start)
SECTIONS
{
    . = 0x10000;
    .text : ALIGN(16) { KEEP(*(.text.entry)) *(.text*) }
    .rodata : ALIGN(16) { *(.rodata*) }
    .data : ALIGN(16) { *(.data*) }
    .bss : ALIGN(16)
    {
        _bss_start = .;
        *(COMMON) *(.bss*)
        _bss_end = .;
    }
    _kernel_end = ALIGN(4096);
    ASSERT(_kernel_end <= 0x90000, "NexOS kernel overlaps the initial boot stack")
}
