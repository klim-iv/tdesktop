## Build instructions for Linux using Docker

### Clone source code

    git clone -b v7_3_3 --recursive https://github.com/klim-iv/tdesktop.git

### Prepare docker image

    cd ./my-configs
    docker build -t tdesktop:7.3.3 .

`Dockerfile` was build with using this [instruction][linux] or by execute:
`Telegram/build/prepare/linux.sh`

### Building the project

Run: `my-configs/build.sh`

[linux]: ../docs/building-linux.md
