# libwinring

[liburing](https://github.com/axboe/liburing) like API implementation for Windows IoRing ( to play with )

## References

* Official demo: https://github.com/yardenshafir/IoRing_Demos
* Explaination: https://windows-internals.com/i-o-rings-when-one-i-o-operation-is-not-enough/


## Bench (output format adjusted, for easier to compare)

`test/bench.cpp`

### Windows IoRing

Win11 22H2 (22616.100), gcc version 12.1.1 20220510 (GCC with MCF thread model, built by LH_Mouse)

```
plain IORING_OP_NOP:    2491105000
this_thread::yield:      708081000
pause:                   398207000
```

### Compare to Linux io_uring

Linux MSIGE76 5.10.102.1-microsoft-standard-WSL2, gcc version 12.1.0 (GCC)

```
plain IORING_OP_NOP:    1427442787
this_thread::yield:     1001010416
pause:                   390992667
```

## License

MIT
