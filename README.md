# NoCompilerNoLinker
No ccompiler No Linker Automatic Executer

This code automatically creates ELF Header ,which is executable file format in Linux, and creates assembly code byte format in text section. then it is a elf format file. 

First:  
```bash
gcc -o Makelf makelf.c
```
it creates **(tiny_hello)** file.

then:  
```bash
chmod +x tiny_hello
```

now it is executable 🎉

and finally:  
```bash
./tiny_hello
```

ta daaaa :D it printed **Hi** on the screen :D without any compilers any linkers any preprocessors :D

