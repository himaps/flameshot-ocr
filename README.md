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
a Debian bullseye container (glibc 2.31, matching the Kylin V20 / V10 SP1
target), then packaged manually with `dpkg-deb` without strict `Depends` — a
UOS/deepin/Kylin desktop already provides the Qt5 runtime. Install with:

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

Pushing a `v*` tag (e.g. `v12.1.0-kylin.1`) builds both architectures and
publishes a GitHub release with the deb and tarball assets (文件名格式：
`flameshot-ocr-<版本>-kylin-<架构>[-offline].deb`); pushing master runs the
same build as a dual-architecture precheck.

发布前工作流强制执行四道质量门禁（细节见踩坑记录 13–16）：

- **glibc 基线门禁**：`objdump` 扫描主程序最高 GLIBC 符号版本，超过 2.31 直接判失败，杜绝"在麒麟上启动即报 `GLIBC_2.3x not found`"的产物出厂；
- **构建后载荷自检**：对每个 deb 跑 `dpkg-deb -c` 断言——常规包必须有主程序和 desktop 文件，离线包另加 `install-deps.sh` 和非空依赖闭包；
- **真实麒麟验证（发布门禁）**：在钉了日期 tag 的真实麒麟容器 `ghcr.io/distrotwin/kylin:v10sp1-base-amd64-<日期>`（github.com/distrotwin/kylin，从麒麟公开 apt 归档自举）里，按离线用户流程装桌面运行库 → `dpkg -i` 离线包 → 跑 `install-deps.sh` → `ldd` 零缺失 → `eng.traineddata` 就位 → `flameshot --version` 可运行。Release 发布以该 job 通过为前置条件；
- **注解诊断**：失败日志尾部重发为 `::error::`/`::warning::` 注解（logs API 对匿名请求 403，check-run 注解是公开 API，本机无凭据也能定位失败），完整日志同时兜底推 `ci-logs` 分支。

## 离线环境安装（offline install）

Release 提供两个 deb 变体，按需二选一（文件名格式：`flameshot-ocr-<版本>-kylin-<架构>[-offline].deb`，架构为 amd64 / aarch64）：

- `flameshot-ocr-<版本>-linux-<架构>.deb` — 常规包，适合已具备 Qt5 运行时的 UOS/deepin 桌面。
- `flameshot-ocr-<版本>-linux-<架构>-offline.deb` — 离线包，在常规内容之外额外打包了运行时依赖闭包（`deps/*.deb`）、安装脚本和英文 Tesseract 训练数据。

安装方式与常规包相同：

    sudo dpkg -i flameshot-ocr-<版本>-linux-<架构>-offline.deb

安装结束时 postinst 会通过 systemd 延迟 10 秒执行一次依赖校验安装（等 dpkg 锁释放，等同手动执行下述脚本，结果在 `journalctl -u flameshot-ocr-deps`）：**没有要装的就直接结束；有缺失库、或系统的 Qt5 比包内版本旧（麒麟 V10 SP1 桌面自带 Qt 5.12，本程序按 5.15 构建），则自动安装/升级**。systemd 不可用时回退为提示手动执行：

    sudo /usr/share/flameshot-ocr/install-deps.sh

脚本逻辑（版本感知、分类施策）：逐个检查 `deps/` 里的依赖包——**Qt5 库（libqt5\*）在系统版本更低时用包内副本升级**（Qt5 小版本间向前 ABI 兼容，升级不影响系统里其他 Qt 应用；反向不兼容，5.15 构建的程序不能跑在 5.12 上，而 `ldd` 只看 soname 看不出这种差别）；**其余库只补缺，不覆盖发行版自带版本**（麒麟对 X11/GL/krb5 栈有自己的定制，bullseye 副本还可能带 `libgl1 (= libglx0)` 这类严格同版本依赖，混装会把系统栈搞坏）。合并成一次 `dpkg -i` 调用，由 dpkg 按依赖顺序配置。`deps/` 是构建时按 Depends 递归展开的完整闭包——麒麟 V20（Ubuntu 20.04 基座）与 Debian 对部分库的包名不同（如 libicu63/libicu66），不同名的库可以共存，逐层展开才能保证离线 `dpkg -i` 不缺依赖。执行完再做一次 `ldd` 校验，明确报告"全部就绪"或仍缺失的库。

OCR 数据说明：离线包内含英文训练数据（`tesseract-ocr-eng`），开箱即可识别英文。其他语言（`tesseract-ocr-chi-sim` 等）是独立的数据包，离线机需要在有网环境下载后手动 `dpkg -i`，或把 `.traineddata` 直接放进 `/usr/share/tesseract-ocr/4.00/tessdata/`。

### 常见问题：运行报 `error while loading shared libraries: libtesseract.so.4`

这是目标机上没有 Qt5/Tesseract 运行库，说明装的是**常规包**，或装了离线包但依赖安装没有执行。处理：

1. 确认装的是 `-offline` 后缀的离线包，且架构与机器一致（`kylin-aarch64` 对应 ARM64）；
2. 执行一次 `sudo /usr/share/flameshot-ocr/install-deps.sh`，看到 `OK: all runtime libraries resolved` 即可正常运行；
3. 若脚本报 `WARNING: these libraries are still missing`，把输出发回来排查。

注意 glibc 边界仍然适用：bullseye 构建产物要求目标机 glibc ≥ 2.31，glibc 更老的系统装了依赖库也无法运行。

两个硬边界：

- **glibc 不能打包替换**。所有产物在 Debian bullseye 容器（glibc 2.31）构建，要求目标机 glibc ≥ 2.31——麒麟 V20 / 优麒麟（Ubuntu 20.04 基座，glibc 2.31-0kylin）✓、UOS/deepin 桌面 ✓、Debian 11+ ✓；老 UOS 20（glibc 2.28）不在支持范围，需要目标机自编译或换更老的基础镜像（Qt/Tesseract 版本会相应变老，API 未必兼容本项目）。
- **postinst 里不能嵌套 `dpkg -i`**。dpkg 全程持有锁，维护脚本执行期间再调 dpkg 会报锁冲突，所以 postinst 只做检测，通过 systemd 延迟执行安装脚本（等同手动执行），systemd 不可用时提示手动运行。

## 构建踩坑记录（amd64 / aarch64 deb）

在 bullseye 容器 + GitHub 原生 runner 上构建 flameshot-ocr 时遇到的问题与解决方法，供后续同类型 Qt 产品参考：

1. **glibc 基线必须匹配目标机**。容器用 bullseye（glibc 2.31，Qt 5.15 + Tesseract 4.1 齐全），构建产物向下兼容一切 ≥ 2.31 的系统。选容器先问"目标机 glibc 多少"。
2. **EOL 基线的 apt 源处于"半归档"状态**。bullseye 过 LTS 后，deb.debian.org 仍提供 `bullseye-security` 的索引（`apt-get update` 正常），但 pool 里的 .deb 已被移走（安装时成片 404）。解决：只把 security 套件指到 `archive.debian.org`，main 保持快速的 deb.debian.org；不要把整个源都切到 archive——它对云厂商 runner IP 限流严重，会造成更难排查的间歇性失败。
3. **deb 版本号必须以数字开头**。本仓库在打第一个 tag 之前 `git describe --tags --always` 只返回纯 git SHA，`dpkg-deb` 直接拒绝；统一加 `0.0.0-` 前缀兜底。打过 `v*` tag 后 master 构建自动变成 `12.1.0-ocr.1-N-g<sha>` 形式，天然合法。
4. **CMake configure 阶段有网络依赖（FetchContent）**。本项目在 configure 时现场克隆 Qt-Color-Widgets（GitLab）和 SingleApplication（GitHub）两个源码库，容器必须装 `git` 且能出外网，首次 configure 耗时主要花在这里；纯离线环境源码编译会直接失败，需预先 `git clone` 后改用本地路径。
5. **OCR 依赖的 pkg-config 模块名**。CMake 找的是 `tesseract` 和 `lept` 两个模块，分别由 `libtesseract-dev` 和 `libleptonica-dev` 提供；bullseye 的 Tesseract 4.1.1 与本项目使用的 TessBaseAPI（`SetImage`/`GetUTF8Text`）完全兼容，无需追求更高版本。
6. **离线闭包必须显式加入 ldd 看不见的运行时依赖**。`ldd` 只能看到启动时链接的共享库，本项目的离线包额外显式 seed 了三类：Tesseract 训练数据（`tesseract-ocr-eng`、`tesseract-ocr-osd`，架构无关数据包，漏掉则库全齐但 OCR 初始化一律失败）；`libqt5svg5`（flameshot 的 SVG 图标靠 Qt 运行时 dlopen 的 `qsvgicon` 插件渲染，Debian 默认 `--as-needed` 让主程序的 DT_NEEDED 里根本没有 libQt5Svg，漏掉则图标全部显示为碎图）；其余 Qt 插件（xcb 平台插件等）由 `libqt5gui5` 的 Depends 闭包带出。验证办法：下载产物 deb，检查 `deps/` 里是否含上述包。
7. **翻译工具链分属两个包**。`Qt5LinguistTools` 的 CMake 配置文件在 `qttools5-dev` 里，`lrelease` 可执行文件在 `qttools5-dev-tools` 里，两个都要装——只装后者会在 configure 期报 `Could not find a package configuration file provided by "Qt5LinguistTools"`；只装前者则构建期静默丢失翻译。
8. **arm64 用原生 runner，不做交叉编译**。`ubuntu-24.04-arm` 对公共仓库免费，容器内直接原生构建，避免交叉工具链和 sysroot 的所有坑。唯一要注意：debian-slim 容器里的 pkg-config 不会自动搜索多架构目录，需要手动设 `PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig`。
9. **容器里的 git 会报"dubious ownership"**。checkout 后工作区属主与容器 root 不一致时，`git describe` 乃至 CMakeLists 自己调用的 `git rev-parse` 都会失败；工作流在构建前统一 `git config --global --add safe.directory "$GITHUB_WORKSPACE"`。
10. **dpkg 全程持锁** → `postinst` 无法嵌套 `dpkg -i`（见"离线环境安装"）。
11. **离线依赖闭包必须递归展开 + 版本感知安装**。目标发行版对同一库可能用不同包名（麒麟/focal 的 libicu66 对 Debian 的 libicu63），只按 `ldd` 收一层依赖在离线机上 `dpkg -i` 会因 Depends 缺名而失败——构建时按 Depends 逐轮展开成完整闭包（不同名的库共存不冲突）；安装时按版本比较，系统库较新则保留、较旧或缺失才装包内的。
12. **包名与二进制名分离**。deb 包名用 `flameshot-ocr` 以区别于发行版仓库的 `flameshot`，但二进制保持 `/usr/bin/flameshot` 不改名——改名会破坏桌面快捷键、脚本和文档里既有的 `flameshot gui` 调用方式。代价是与发行版 flameshot 包存在文件冲突，安装前需先卸载它。
13. **构建后必须做载荷自检**。离线包构建步骤多（staging 拷贝、闭包下载、脚本生成），任何一步漏掉都会产出"看起来正常"的包——同门项目 pdfcraft 出过离线包只装了依赖闭包、没有应用本体的退化包（靠人眼对比尺寸才抓出）。现在工作流对每个 deb 跑 `dpkg-deb -c` 断言：常规包必须有主程序和 desktop 文件，离线包另加 `install-deps.sh` 和足量依赖 deb。注意 `dpkg-deb -c` 输出先落文件再 grep——**不要 `grep -q` 于管道**（pipefail 下首个匹配即退出，SIGPIPE 杀掉 dpkg-deb 生产端，匹配了也误报失败）。
14. **postinst 对载荷缺失要大声报错**。若 `/usr/bin/flameshot` 不在，`ldd` 会静默输出空结果，"无缺失库"的判定会把坏包当健康包放行；脚本开头显式检查主程序存在，缺失即 `exit 1`，让 dpkg 把包标记为配置失败而不是假装装好了。
15. **真实麒麟容器验证 + 钉日期 tag**。bullseye 只是 glibc 同基线的近似，"运行库假设"（包名与版本在麒麟源里成立）要靠真实归档检验：distrotwin/kylin 容器从麒麟公开 apt 源自举（`v10sp1` = 桌面 V10 SP1 2503，glibc 2.31 / GCC 9.3 / Ubuntu 20.04 血脉）。CI 必须钉带日期的 tag（如 `v10sp1-base-amd64-20260903`）——滚动 tag 会随重建漂移，`latest` 甚至会跨大版本把 glibc 从 2.31 推到 2.38。验证按离线用户真实路径走：装麒麟桌面运行库（同时验证了包名假设）→ `dpkg -i` 离线包 → 手动跑 `install-deps.sh`（容器无 systemd，恰好走文档写的手动回退路径）→ `ldd` 零缺失 → `eng.traineddata` 就位 → `flameshot --version`。Release 发布以该 job 通过为前置条件。
16. **无凭据也能诊断 CI**。Actions logs API 对匿名请求 401/403，job 页面日志靠前端加载——本机无 gh CLI / token 时两者都读不到；**check-run annotations 是公开 API**。工作流在失败时把日志最后 30 行重发为 10 notice + 10 warning + 10 error 注解（每级每步上限 10 条，title 带"距末尾第几行"防合并），用 `commits/{sha}/check-runs` 匿名可读；静默 `set -e` 死亡用 `set -x` + `PS4='+L$LINENO: '` 让失败命令自己报行号。完整日志兜底推 `ci-logs` 分支。
17. **离线闭包的安装策略必须分类施策（真实麒麟容器里炸出来的）**。bullseye 全量"版本旧就升级"在麒麟 V10 SP1 上会坏事：麒麟对 X11/GL/krb5 栈有自己的定制，bullseye 副本盖上去时 mesa 的严格同版本依赖（`libgl1 (= libglx0)`）在单次 `dpkg -i` 里级联失败 16 个包。正确策略：**Qt5 库（libqt5\*）系统更旧就升级**（麒麟 V10 SP1 桌面 UKUI 自带 Qt 5.12.8，本程序按 5.15 构建，反向 ABI 不兼容；Qt5 向前兼容，升级不伤其他 Qt 应用）；**其余包只补缺、永不覆盖**已安装版本；**GCC 运行时（libgomp1）根本不进闭包**——bullseye 副本带 `gcc-10-base (= 10.2.1-6)` 严格同版本依赖，在麒麟上必然配置失败并级联 libqt5gui5/libtesseract4。postinst 也不能用 `ldd` 门控——soname 能解析不代表符号版本够（`ldd` 看不出 `Qt_5.15` 缺失），所以无条件延迟 10 秒跑版本感知的安装脚本，无事时它自己秒退。最后，安装包内 Qt5 时要 `dpkg -i --force-depends-version`：bullseye 的包把构建时的 X11 版本写进了版本子句（`libx11-xcb1 (>= 2:1.7.2)`），麒麟自带的定制版 1.6.9 名字满足、运行时也无碍，版本约束换个发行版就没有意义——运行时可行性由 verify-on-kylin 的 ldd 门禁和实际执行把关。

## 复用本工作流（其他 Qt 项目）

复制 `.github/workflows/release.yml` 后按项目调整：

- 顶层产物名、control 里的 `Package`/`Description`、发布说明文本、`deps/` 与安装脚本的路径（全局替换 `flameshot-ocr` 即可）
- "Install build dependencies" 换成项目真实的构建依赖；先在 packages.debian.org 按容器基线（当前 bullseye）确认是否有对应 `-dev` 包
- 项目若也有"ldd 看不见的运行时数据"（词典、模型、资源包），参照离线包构建步骤里显式 seed 的做法加入闭包
- glibc 门禁、载荷自检、注解诊断、ci-logs 兜底这四样与产品无关，原样保留即可
- 目标机是麒麟 V10 SP1/V20 时保留 verify-on-kylin job（真实归档验证 + 发布门禁），按指南要求更新容器 tag 里的日期
- 推送 master 做双架构预检；打 `v*` tag 自动构建并发布 Release

---

## License

This project is licensed under GPLv3, same as the original Flameshot.

---


Refer to [the original Flameshot documentation](https://flameshot.org/docs/) for more details about flameshot.
