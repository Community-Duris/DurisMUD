# Docker Build Environment

Use this environment to compile DurisMUD in the Linux target environment instead
of relying on host-machine compiler packages.

## Build the Image

```sh
docker compose -f docker-compose.build.yml build
```

The image installs:

- `build-essential`
- `default-libmysqlclient-dev`
- `libcrypt-dev`
- `libxml2-dev`
- `zlib1g-dev`
- `make`
- `pkg-config`

## Compile

```sh
docker compose -f docker-compose.build.yml run --rm build
```

For a clean rebuild:

```sh
docker compose -f docker-compose.build.yml run --rm build /bin/sh -lc \
  "cp Makefile.linux Makefile && cp ships/Makefile.linux ships/Makefile && make clean && make dms_new"
```

The compose command copies `src/Makefile.linux` to `src/Makefile` and
`src/ships/Makefile.linux` to `src/ships/Makefile` inside the mounted workspace
before building.
