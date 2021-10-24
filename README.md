# libwinring

[liburing](https://github.com/axboe/liburing) like API implementation for Windows IoRing ( to play with )

## References

* Official demo: https://github.com/yardenshafir/IoRing_Demos
* Explaination: https://windows-internals.com/i-o-rings-when-one-i-o-operation-is-not-enough/


## Bench (output format adjusted, for easier to compare)

### Windows IoRing

test/bench.cpp

Win11 22000.282, VS2022 preview 6

```
plain IORING_OP_NOP:  3061447800
this_thread::yield:    574536900
pause:                 415972400
```

### Compare to Linux io_uring

https://github.com/CarterLi/liburing4cpp/blob/master/io_uring/bench.cpp

Linux MSI 5.10.60.1-microsoft-standard-WSL2, g++ 11.1.0

```
plain IORING_OP_NOP:  1528107815
this_thread::yield:    974317628
pause:                 422511241
```

## License

MIT
