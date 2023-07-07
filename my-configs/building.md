## Build instructions for Linux using Docker

### Clone source code

    git clone -b v4_8_4 --recursive https://github.com/telegramdesktop/tdesktop.git

### Prepare docker image

    cd ./my-configs
    docker build -t tdesktop:4.8.4 .

`Dockerfile` was build with using this [instruction][linux]

### Building the project

Run: `my-configs/build.sh`

[linux]: ../docs/building-linux.md
