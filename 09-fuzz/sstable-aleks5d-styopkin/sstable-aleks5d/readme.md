## Сборка

```
clang++ -std=c++17 -g -fsanitize=fuzzer,address fuzz.cc precheck.h convert.cpp
```

## Запуск

```
./a.out -max_len=10000
```

## Ошибки и фиксы

```
==2541331==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7ffe6a793a60 at pc 0x55bb2f1e075c bp 0x7ffe6a793200 sp 0x7ffe6a7929b0
READ of size 128 at 0x7ffe6a793a60 thread T0
    #0 0x55bb2f1e075b in MemcmpInterceptorCommon(void*, int (*)(void const*, void const*, unsigned long), void const*, void const*, unsigned long) (/home/styopkin/decentralized/practice/09-fuzz/kaurkerdevourer/sstable-aleks5d/a.out+0x7c75b) (BuildId: 7354f4e4235551f9337ef3f00edaed9ff454339f)
    #1 0x55bb2f1e0b09 in __interceptor_memcmp (/home/styopkin/decentralized/practice/09-fuzz/kaurkerdevourer/sstable-aleks5d/a.out+0x7cb09) (BuildId: 7354f4e4235551f9337ef3f00edaed9ff454339f)
    #2 0x55bb2f287c7e in LLVMFuzzerTestOneInput /home/styopkin/decentralized/practice/09-fuzz/kaurkerdevourer/sstable-aleks5d/fuzz.cc:32:20
    #3 0x55bb2f1ac883 in fuzzer::Fuzzer::ExecuteCallback(unsigned char const*, unsigned long) (/home/styopkin/decentralized/practice/09-fuzz/kaurkerdevourer/sstable-aleks5d/a.out+0x48883) (BuildId: 7354f4e4235551f9337ef3f00edaed9ff454339f)
    #4 0x55bb2f1abfd9 in fuzzer::Fuzzer::RunOne(unsigned char const*, unsigned long, bool, fuzzer::InputInfo*, bool, bool*) (/home/styopkin/decentralized/practice/09-fuzz/kaurkerdevourer/sstable-aleks5d/a.out+0x47fd9) (BuildId: 7354f4e4235551f9337ef3f00edaed9ff454339f)
    #5 0x55bb2f1ad7c9 in fuzzer::Fuzzer::MutateAndTestOne() (/home/styopkin/decentralized/practice/09-fuzz/kaurkerdevourer/sstable-aleks5d/a.out+0x497c9) (BuildId: 7354f4e4235551f9337ef3f00edaed9ff454339f)
    #6 0x55bb2f1ae345 in fuzzer::Fuzzer::Loop(std::vector<fuzzer::SizedFile, std::allocator<fuzzer::SizedFile> >&) (/home/styopkin/decentralized/practice/09-fuzz/kaurkerdevourer/sstable-aleks5d/a.out+0x4a345) (BuildId: 7354f4e4235551f9337ef3f00edaed9ff454339f)
    #7 0x55bb2f19c482 in fuzzer::FuzzerDriver(int*, char***, int (*)(unsigned char const*, unsigned long)) (/home/styopkin/decentralized/practice/09-fuzz/kaurkerdevourer/sstable-aleks5d/a.out+0x38482) (BuildId: 7354f4e4235551f9337ef3f00edaed9ff454339f)
    #8 0x55bb2f1c6172 in main (/home/styopkin/decentralized/practice/09-fuzz/kaurkerdevourer/sstable-aleks5d/a.out+0x62172) (BuildId: 7354f4e4235551f9337ef3f00edaed9ff454339f)
    #9 0x7fc28dbaed8f in __libc_start_call_main csu/../sysdeps/nptl/libc_start_call_main.h:58:16
    #10 0x7fc28dbaee3f in __libc_start_main csu/../csu/libc-start.c:392:3
    #11 0x55bb2f190ec4 in _start (/home/styopkin/decentralized/practice/09-fuzz/kaurkerdevourer/sstable-aleks5d/a.out+0x2cec4) (BuildId: 7354f4e4235551f9337ef3f00edaed9ff454339f)

Address 0x7ffe6a793a60 is located in stack of thread T0 at offset 2112 in frame
    #0 0x55bb2f28764f in LLVMFuzzerTestOneInput /home/styopkin/decentralized/practice/09-fuzz/kaurkerdevourer/sstable-aleks5d/fuzz.cc:10

  This frame has 11 object(s):
    [32, 544) 'outFile' (line 18)
    [608, 640) 'fileName' (line 22)
    [672, 673) 'ref.tmp' (line 22)
    [688, 720) 'agg.tmp'
    [752, 784) 'newFileName' (line 25)
    [816, 817) 'ref.tmp12' (line 25)
    [832, 864) 'agg.tmp15'
    [896, 928) 'agg.tmp18'
    [960, 1480) 'inFile' (line 29)
    [1616, 2008) 'buffer' (line 30)
    [2080, 2112) 'ref.tmp34' (line 32) <== Memory access at offset 2112 overflows this variable
```

  Добавил в конец всех функций закрытие файлов, о котором забыли.
  После 3 часов теста, больше ошибок найдено не было.