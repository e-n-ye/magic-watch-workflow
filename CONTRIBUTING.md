# Contributing

本仓库的目标之一是把嵌入式开发流程本身跑通。所有修改都遵循下面的顺序：

```text
branch --> code change --> Build + Cppcheck --> commit --> push
  --> pull request --> GitHub Actions (build + analysis)
  --> Merge or Rework
```

## 1. Branch

从最新的 `main` 创建工作分支。禁止直接在 `main` 上开发或推送。

```sh
git fetch origin
git switch -c <type>/<short-name> origin/main
```

分支完成后保持一个明确的主题，例如 `build/f411-ci` 或 `docs/workflow-rules`。

## 2. Code change

先阅读相关代码、目录边界和已有文档，再进行最小范围修改。一个提交只解决一个清晰的问题。

CubeMX 工程遵循最小化修改原则：不直接编辑 `.ioc`；需要改变 CubeMX 配置时，在 CubeMX 中修改并重新生成；手写代码放在现有 `user/` 和用户维护的 CMake 入口中。

## 3. Build + Cppcheck

涉及 F411 固件、构建脚本、工具链或 CI 的修改，提交前必须在本地运行：

```sh
cd firmware/stm32/f411_watch
cmake --preset Debug
cmake --build --preset Debug
cmake --build --preset Debug --target cppcheck
```

文档-only 修改可以由路径过滤跳过 F411 构建，但仍必须让 PR 的 `CI / CI Gate` 成功结束。若修改可能影响构建，按 F411 命令完整验证。

## 4. Commit + push

先确认工作树和差异只包含当前主题，然后提交：

```sh
git status
git diff --check
git add <files>
git commit -m "<type>:<scope>:<description>"
git push -u origin <branch>
```

提交信息使用约定式格式，例如：

```text
build:f411:connect hand-written user target
fix:ci:match cppcheck version output
docs:workflow:document guarded delivery flow
```

## 5. Pull request and CI

创建目标为 `main` 的 Pull Request，检查变更范围和提交历史。等待 GitHub Actions 完成：

- F411 相关修改：Debug configure、build 和 Cppcheck；
- 文档-only 修改：F411 job 可以按路径规则跳过；
- 所有 PR：`CI / CI Gate` 必须成功。

CI 失败时回到工作分支修复，重新执行本地验证、提交和推送。不要通过删除检查、管理员绕过或直接推送来合并。

## 6. Merge or rework

只有在检查通过、讨论已解决且分支为最新 `main` 时才合并。仓库只允许 `Rebase and merge`，不使用 merge commit 或 squash merge。合并后可以删除已经完成的工作分支，`main` 中的提交不会受到影响。

GitHub 的 `protect-main` ruleset 和 `CI / CI Gate` 是服务器端的最终门禁；本文件和根目录 `AGENTS.md` 是协作约定与 AI 读取入口。
