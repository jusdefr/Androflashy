ENTRY(_start)
SECTIONS
{
    . = 0x10000;
    __image_start = .;
    .text : ALIGN(16) { KEEP(*(.text.entry)) *(.text*) }
    .rodata : ALIGN(16) { *(.rodata*) }
    .data : ALIGN(16) { *(.data*) __image_data_end = .; }
    .bss : ALIGN(16)
    {
        _bss_start = .;
        *(COMMON) *(.bss*)
        _bss_end = .;
    }
    _kernel_end = ALIGN(4096);
    ASSERT(_kernel_end <= 0x40000, "NexOS kernel exceeds the 300-sector loader budget")
}
