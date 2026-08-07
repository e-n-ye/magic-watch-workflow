# F411 CI 镜像

这个目录只维护构建环境，不复制项目源码。构建时把本目录作为 Docker context；CI 或本地运行时再把仓库挂载到 `/workspace`。

## 固定工具

- Ubuntu 24.04：`sha256:561618e2c15bf2397621dd04f96926663a3b5616c189cf7e38db7e82f5c538ea`
- CMake `3.31.4`
- Ninja `1.13.2`
- Arm GNU Toolchain `13.3.Rel1` (`95c011cee430e64dd6087c75c800f04b9c49832cc1000127a92a97f9c8d83af4`)
- Cppcheck `2.21.0`

所有上游归档在 Dockerfile 中使用 SHA-256 校验。

## 本地构建

```sh
docker build \
  --tag enen001/magic-watch-f411-ci:13.3-rel1 \
  tools/docker/f411-ci
```

构建后记录实际大小和层组成：

```sh
docker image inspect enen001/magic-watch-f411-ci:13.3-rel1
docker history enen001/magic-watch-f411-ci:13.3-rel1
```

## 使用镜像验证 F411

```sh
docker run --rm \
  --volume "${PWD}:/workspace" \
  --workdir /workspace/firmware/stm32/f411_watch \
  enen001/magic-watch-f411-ci:13.3-rel1 \
  sh -c 'cmake --preset Debug && cmake --build --preset Debug && cmake --build --preset Debug --target cppcheck'
```

Docker Hub 推送后，GitHub Actions 必须使用该镜像的 digest 引用，不使用 `latest`。

推送完成后，在仓库 `Settings -> Secrets and variables -> Actions -> Variables` 中设置：

```text
F411_CI_IMAGE=docker.io/enen001/magic-watch-f411-ci@sha256:<64 位小写摘要>
```

Actions 会拒绝没有 digest 的镜像引用。Docker Hub 登录可使用 Docker Desktop 欢迎页的 GitHub OAuth 按钮；推送前先执行 `docker login`。
