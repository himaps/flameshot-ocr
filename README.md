# Flameshot OCR

**A fork of Flameshot with built-in OCR: Instantly extract and copy text from screenshots to your clipboard using Tesseract OCR.**

---

## Features

- Take screenshots and extract text from any region using Tesseract OCR
- Copy recognized text directly to your clipboard
- Works fully offline, all OCR is performed locally
- Supports multiple languages (if Tesseract language packs are installed)
- All core Flameshot screenshot and annotation features are preserved

---

## Usage

1. **Run Flameshot OCR:**

```bash
./build/src/flameshot
```
2. **Take a screenshot as usual (tray icon or hotkey).**
3. **Select the OCR tool** from the toolbar.
4. **Draw/select a region** containing text.
5. **Text is automatically copied to your clipboard.**

---

## Keyboard Shortcut (Linux)

To bind OCR capture to <kbd>Print Screen</kbd>:

- Go to **Settings → Keyboard → Shortcuts → Custom Shortcuts**
- Add a new shortcut with command:

```bash
flameshot gui
```

- Assign it to <kbd>Print Screen</kbd>

For best results, add `flameshot` to your startup applications so the daemon is running.

---

## Compilation

### **Dependencies**

- Qt >= 5.9
- GCC >= 7.4
- CMake >= 3.12
- Tesseract OCR (and language packs as needed)


#### **Ubuntu/Debian example:**

```bash
sudo apt update
sudo apt install g++ cmake build-essential qtbase5-dev qttools5-dev-tools libqt5svg5-dev \
libtesseract-dev libleptonica-dev tesseract-ocr
```


#### **Optional: Install extra language packs**

```bash
sudo apt-get install \
tesseract-ocr-deu \
tesseract-ocr-fra \
tesseract-ocr-spa \
tesseract-ocr-chi-sim \
tesseract-ocr-jpn \
tesseract-ocr-ara \
tesseract-ocr-hin \
tesseract-ocr-rus
```


### **Build**

```bash
git clone https://github.com/sid-sg/flameshot-ocr.git
cd flameshot-ocr
mkdir build
cmake -S . -B build && cmake --build build --parallel
```


---

## Notes

- OCR language selection defaults to English. To use other languages, install the relevant Tesseract language packs.
- For best clipboard compatibility, run `flameshot` as a daemon and use the tray icon or hotkey.
  
---

## Deb packages (UOS / deepin / Kylin)

Releases are built on GitHub Actions with native runners (amd64 + arm64) inside
a **real Kylin V10 SP1 container** (`ghcr.io/distrotwin/kylin`, bootstrapped
from Kylin's public apt archives — compiler, Qt 5.12 and Tesseract are exactly
what a Kylin machine sees), then packaged manually with `dpkg-deb` without
strict `Depends` — a UOS/deepin/Kylin desktop already provides the desktop
runtime. Install with:

    sudo dpkg -i flameshot-ocr-*.deb

The package name is `flameshot-ocr` but the binary keeps the upstream name
`flameshot` (so the hotkey docs above keep working). If a distro `flameshot`
package is installed, dpkg refuses with a file conflict — remove it first:

    sudo apt remove flameshot && sudo dpkg -i flameshot-ocr-*.deb

To build a deb by hand:

    sudo apt install g++ cmake pkg-config git qtbase5-dev libqt5svg5-dev \
      qttools5-dev qttools5-dev-tools libtesseract-dev libleptonica-dev
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
    make -C build -j$(nproc)
    make -C build install DESTDIR="$PWD/staging"
    # copy staging/. into pkg/flameshot-ocr_<version>_<arch>/ with a DEBIAN/control,
    # then: dpkg-deb --build --root-owner-group pkg/flameshot-ocr_<version>_<arch>

Pushing a `v*` tag (e.g. `v12.1.0-kylin.2`) builds both architectures and
publishes a GitHub release with two deb variants per arch（文件名格式：
`flameshot-ocr-<版本>-kylin-<架构>[-offline].deb`）；pushing master runs the
same build as a dual-architecture precheck. OCR 识别语言为**中文+英文**
（`chi_sim+eng`），离线包已内置两国的 traineddata；只有英文数据时自动回退纯英文。

发布前工作流强制执行四道质量门禁（细节见踩坑记录）：

- **glibc 基线门禁**：`objdump` 扫描主程序最高 GLIBC 符号版本，超过 2.31 直接判失败，杜绝"在麒麟上启动即报 `GLIBC_2.3x not found`"的产物出厂；
- **构建后载荷自检**：对每个 deb 跑 `dpkg-deb -c` 断言主程序与 desktop 文件确实在包里（离线包另加安装脚本、中文数据包与非空依赖闭包）；
- **真实麒麟验证（发布门禁）**：在钉了日期 tag 的干净麒麟 base 容器 `ghcr.io/distrotwin/kylin:v10sp1-base-amd64-<日期>`（github.com/distrotwin/kylin，从麒麟公开 apt 归档自举）里，**只装离线包并跑包内依赖安装脚本**（不从外部归档补任何运行库，专治闭包不全）→ `ldd` 零缺失 → `chi_sim`/`eng` traineddata 就位 → `flameshot --version` 可运行。Release 发布以该 job 通过为前置条件；
- **注解诊断**：失败日志尾部重发为 `::error::`/`::warning::` 注解（logs API 对匿名请求 403，check-run 注解是公开 API，本机无凭据也能定位失败），完整日志同时兜底推 `ci-logs` 分支。

glibc 硬边界：产物要求目标机 glibc ≥ 2.31——麒麟 V20 / V10 SP1（Ubuntu 20.04 基座）✓、UOS/deepin 桌面 ✓、Debian 11+ ✓；老 UOS 20（glibc 2.28）不在支持范围（glibc 永远不能打包替换），需要目标机自编译。

## 离线环境安装（offline install）

离线机装 `-offline` 后缀的 deb（架构对好：`kylin-aarch64` 对应 ARM64）：

    sudo dpkg -i flameshot-ocr-<版本>-kylin-<架构>-offline.deb

安装结束时 postinst 会通过 systemd 延迟 10 秒执行一次依赖安装（等 dpkg 锁释放，结果在 `journalctl -u flameshot-ocr-deps`）；systemd 不可用时回退为提示手动执行：

    sudo /usr/share/flameshot-ocr/install-deps.sh

脚本逻辑（版本感知、分类施策）：**Qt5 库（libqt5\*）系统版本更低时用包内副本升级**（Qt5 向前 ABI 兼容，升级不伤系统里其他 Qt 应用）；**其余库只补缺，不覆盖发行版自带版本**。包内闭包（`deps/`）与构建同源（麒麟公开归档），含 Qt5、Tesseract 与**中文+英文 traineddata**，装完自动 `ldd` 复验并报告"全部就绪"或仍缺失清单。

OCR 排查：每次识别尝试都会在家目录 `~/flameshot-ocr-ocr-log.txt` 追加分步日志（tesseract 版本、各候选 tessdata 路径的 Init 结果与数据文件存在性），**失败也先写后执行**——日志最后一行就是出事现场。识别不工作时把该文件发回来即可定位。

## 构建踩坑记录（构建环境演变：bullseye → 真实麒麟容器）

构建环境从 Debian bullseye 近似基线迁到真实麒麟 V10 SP1 容器（`ghcr.io/distrotwin/kylin`，devel 档编译、base 档验证），期间在 GitHub 原生 runner 上遇到的问题与解决方法，供后续同类型 Qt 产品参考：

1. **glibc 基线必须匹配目标机**。构建环境最终选了真实麒麟 V10 SP1 容器（glibc 2.31、GCC 9.3、Qt 5.12 原生），产物向下兼容一切 ≥ 2.31 的系统；早期曾用 Debian bullseye（同为 glibc 2.31）作近似基线，后来按麒麟适配指南换成真容器。选容器先问"目标机 glibc 多少"。
2. **deb 版本号必须以数字开头**。本仓库在打第一个 tag 之前 `git describe --tags --always` 只返回纯 git SHA，`dpkg-deb` 直接拒绝；统一加 `0.0.0-` 前缀兜底。打过 `v*` tag 后 master 构建自动变成 `12.1.0-kylin.1-N-g<sha>` 形式，天然合法。
3. **CMake configure 阶段有网络依赖（FetchContent）**。本项目在 configure 时现场克隆 Qt-Color-Widgets（GitLab）和 SingleApplication（GitHub）两个源码库，容器必须装 `git` 且能出外网，首次 configure 耗时主要花在这里；纯离线环境源码编译会直接失败，需预先 `git clone` 后改用本地路径。
4. **OCR 依赖的 pkg-config 模块名**。CMake 找的是 `tesseract` 和 `lept` 两个模块，分别由 `libtesseract-dev` 和 `libleptonica-dev` 提供；麒麟 V10 SP1（Ubuntu 20.04 血脉）与 Debian bullseye 的 Tesseract 都是 4.1.1，与本项目使用的 TessBaseAPI（`SetImage`/`GetUTF8Text`）完全兼容，无需追求更高版本。
5. **翻译工具链分属两个包**。`Qt5LinguistTools` 的 CMake 配置文件在 `qttools5-dev` 里，`lrelease` 可执行文件在 `qttools5-dev-tools` 里，两个都要装——只装后者会在 configure 期报 `Could not find a package configuration file provided by "Qt5LinguistTools"`；只装前者则构建期静默丢失翻译。
6. **arm64 用原生 runner，不做交叉编译**。`ubuntu-24.04-arm` 对公共仓库免费，容器内直接原生构建，避免交叉工具链和 sysroot 的所有坑。唯一要注意：容器里的 pkg-config 不会自动搜索多架构目录，需要手动设 `PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig`。
7. **容器里的 git 会报"dubious ownership"**。checkout 后工作区属主与容器 root 不一致时，`git describe` 乃至 CMakeLists 自己调用的 `git rev-parse` 都会失败；工作流在构建前统一 `git config --global --add safe.directory "$GITHUB_WORKSPACE"`。
8. **包名与二进制名分离**。deb 包名用 `flameshot-ocr` 以区别于发行版仓库的 `flameshot`，但二进制保持 `/usr/bin/flameshot` 不改名——改名会破坏桌面快捷键、脚本和文档里既有的 `flameshot gui` 调用方式。代价是与发行版 flameshot 包存在文件冲突，安装前需先卸载它。
9. **构建后必须做载荷自检，且不要 `grep -q` 于管道**。对每个 deb 跑 `dpkg-deb -c` 断言主程序与 desktop 文件在包里——打包步骤多，任何一步漏掉都会产出"看起来正常"的包（同门项目 pdfcraft 出过载荷缺失的退化包，靠人眼对比尺寸才抓出）。同时 `dpkg-deb -c` 输出要先落文件再 grep：pipefail 下管道里的 `grep -q` 首个匹配即退出，SIGPIPE 杀掉 dpkg-deb 生产端，匹配了也误报失败。
10. **真实麒麟容器验证 + 钉日期 tag**。distrotwin/kylin 容器从麒麟公开 apt 源自举（`v10sp1` = 桌面 V10 SP1 2503，glibc 2.31 / GCC 9.3 / Ubuntu 20.04 血脉），分 micro / base / devel 三档。CI 必须钉带日期的 tag（如 `v10sp1-devel-amd64-20260903`）——滚动 tag 会随重建漂移，`latest` 甚至会跨大版本把 glibc 从 2.31 推到 2.38。验证按用户真实路径走：从麒麟归档装桌面运行库（同时验证了"桌面自带运行库"的包名假设）→ `dpkg -i` deb → `ldd` 零缺失 → `flameshot --version`。Release 发布以该 job 通过为前置条件。
11. **无凭据也能诊断 CI**。Actions logs API 对匿名请求 401/403，job 页面日志靠前端加载——本机无 gh CLI / token 时两者都读不到；**check-run annotations 是公开 API**。工作流在失败时把日志最后 30 行重发为 10 notice + 10 warning + 10 error 注解（每级每步上限 10 条，title 带"距末尾第几行"防合并），用 `commits/{sha}/check-runs` 匿名可读；静默 `set -e` 死亡用 `set -x` + `PS4='+L$LINENO: '` 让失败命令自己报行号。完整日志兜底推 `ci-logs` 分支。
12. **为什么最终换掉 bullseye：跨发行版近似是一整类坑**。bullseye 时期踩过的：麒麟桌面 UKUI 自带 Qt 5.12.8，bullseye Qt 5.15 构建的程序反向 ABI 不兼容，而 `ldd` 只看 soname、看不出符号版本差异；bullseye 包把构建时版本写进版本子句（`libx11-xcb1 (>= 2:1.7.2)`、mesa `libgl1 (= libglx0)`、gcc 运行时的 `gcc-10-base (= ...)`），盖到麒麟自己的定制栈上必然配置失败并级联。教训：**运行库要跟目标机同源**。按麒麟适配指南换成 `v10sp1-devel` 容器后，编译、依赖、验证同宗，这一类问题整体消失；上面留存的坑多与构建环境无关，换基线前别扔。
13. **预置了数据包不等于用得上：OCR 语言组合必须与包内数据一致**。ocrtool 原来先尝试 9 语言组合（`eng+deu+fra+spa+chi_sim+jpn+ara+hin+rus`），失败才回退纯英文——离线包只预置中文+英文时，组合里缺 deu/fra 等会整体初始化失败，**静默回退英文，中文数据躺在包里也识别不了中文**。现在固定请求 `chi_sim+eng`（与离线包内置数据一致），缺中文数据时回退纯英文。给离线场景加任何运行时资源，都要顺路核对消费它的代码路径真的会用到它。

## 复用本工作流（其他 Qt 项目）

复制 `.github/workflows/release.yml` 后按项目调整：

- 顶层产物名、control 里的 `Package`/`Description`、发布说明文本、离线包 `deps/` 与安装脚本路径（全局替换 `flameshot-ocr` 即可）
- "Install build dependencies" 换成项目真实的构建依赖；按容器基线（当前麒麟 V10 SP1，Ubuntu 20.04 包名体系）确认归档里有对应 `-dev` 包
- 离线闭包里 ldd 看不见的运行时资源（数据包、词典、dlopen 的插件），照离线包构建步骤里 seed 的做法显式加入，并核对消费代码真的会用到
- verify-on-kylin 保持"只装离线包 + 包内脚本、不从外部归档补库"的口径，才能暴露闭包不全
- glibc 门禁、载荷自检、注解诊断、ci-logs 兜底这四样与产品无关，原样保留即可
- 目标机是麒麟 V10 SP1/V20 时保留 verify-on-kylin job（真实归档验证 + 发布门禁），按指南要求更新容器 tag 里的日期
- 推送 master 做双架构预检；打 `v*` tag 自动构建并发布 Release

---

## License

This project is licensed under GPLv3, same as the original Flameshot.

---


Refer to [the original Flameshot documentation](https://flameshot.org/docs/) for more details about flameshot.
