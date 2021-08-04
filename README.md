# sed

Implementation of the `sed` command following the [POSIX specification][1] (`man
1p sed`).

## Usage

```sh
$ make
$ echo 'bonjour' | ./sed 'y/uor/foo/'
```

## Test

I use the [Criterion][2] library to unit test.

```sh
make test_run
```

## Coverage

You have to install [gcovr][3] first.

```sh
make report
```

[1]: https://www.man7.org/linux/man-pages/man1/sed.1p.html
[2]: https://github.com/Snaipe/Criterion
[3]: https://github.com/gcovr/gcovr
