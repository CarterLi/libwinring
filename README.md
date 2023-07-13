# libwinring

[liburing](https://github.com/axboe/liburing) like API implementation for Windows IoRing ( to play with )

## References

* Demos: https://github.com/yardenshafir/IoRing_Demos
* Explaination: https://windows-internals.com/i-o-rings-when-one-i-o-operation-is-not-enough/


## Bench (output format adjusted, for easier to compare)

`demo/bench.cpp`

### Windows IoRing

Win11 22H2 (22631.1972), clang version 16.0.5

```
plain IORING_OP_NOP:    2624780000
this_thread::yield:      836409700
pause:                   488763800
```

### Compare to Linux io_uring

Linux MSIGE76 5.15.90.2-microsoft-standard-WSL2, clang version 15.0.7

```
plain IORING_OP_NOP:    1248633820
this_thread::yield:     1113853264
pause:                   433441058
```

## License

MIT
