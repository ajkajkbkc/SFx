@echo off
echo AXF to BIN file generation ...

"D:\Keil_v5\ARM\ARMCC\bin\fromelf.exe" --bin --output ..\MDK-ARM\Output\Neso.bin ..\MDK-ARM\Output\Neso.axf 

echo AXF to BIN file generation completed.

