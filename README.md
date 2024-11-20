# maid

C99 rewrite of [maid](https://github.com/rniii/maid). Because I could.

## Tasks

<!-- maid-tasks -->

### setup

Prepare for building the project.

```sh
meson setup build --wipe
```

### build

```sh
[ -d build ] || maid setup
ninja -C build
```

### run

```sh
maid build
./build/maid
```
