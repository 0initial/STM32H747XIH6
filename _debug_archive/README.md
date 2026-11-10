# _debug_archive — STM32H747XIH6 调试资料归档

本文件夹存放 2026-11-08 ~ 2026-11-10 几次调试过程中产生的脚本、原理图截图和日志。
（原先散落在 C 盘 DSH 工作区，已整体迁移到这里。）

## scripts/

| 文件 | 用途 |
|---|---|
| `fix_wwdg.ps1` | 06_Watch_dog 工程：把 WWDG 参数从 `Counter=0x40`（会立即复位）改为 `Prescaler=128 / Counter=127 / Window=104`，并重新编译 |
| `check_makefile.ps1` | 检查 `Makefile/CMx/Makefile` 的 `C_SOURCES` 里列的 `.c` 是否都真实存在（07_RTC 的 `dma.c` 缺失问题就是它查出来的） |
| `render_pdf.ps1` / `render_pdf2.ps1` / `render_pdf3.ps1` | 用 Windows 自带的 `Windows.Data.Pdf` 把原理图 PDF 渲染成 PNG。**`render_pdf3.ps1` 是最终可用版**，另两个是失败尝试（`StorageFile` 方式读不到文件） |

`check_makefile.ps1` 需要绕过执行策略运行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\check_makefile.ps1 -Makefile ..\..\07_RTC\test\Makefile\CM7\Makefile
```

或者直接用这一行（在工程根目录执行）：

```powershell
$d='Makefile\CM7'; $s=[regex]::Matches((Get-Content "$d\Makefile" -Raw),'\.\./[^\s\\]+\.c')|%{$_.Value}; $s|?{!(Test-Path (Join-Path $d $_))}
```

## schematic/

慧勤智远 STM32H747XIH6 CB V1.2 小系统板原理图（原文件在
`D:\01_Study_Materials\MCU基础\慧勤智远 STM32H747XIH6 CB V1.2小系统板\2. 原理图\`）。

- `sch.pdf` —— 原件副本（6 页）
- `text/` —— 提取出来的文字。`page*.txt` 是保留版面的版本，能看出元件相对位置；`sch.txt` 是顺序提取版
- `render/page1..6.png` —— 整页渲染图，2600px 宽
- `crop/` —— 关键局部裁剪：
  - `crop_key2.png`：KEY 区（WKUP / KEY0 / RESET / **BOOT0** 四个按键 + 上拉电阻）
  - `crop_swd.png`、`crop_key.png`：SWD 4 针排针 + BOOT0 按键
  - `crop_com.png`：COM 区（CH340X 接 USART1）

### 从原理图得到的几个结论（备查）

1. **SWD 排针只有 4 针**：`3.3V / SWDIO / SWCLK / GND`，**没有 NRST**。
   所以 `connect_assert_srst` 不可用；`stm32h7x.cfg` 本身也写明 H7 不支持该模式，复位走 `SYSRESETREQ`。
2. **BOOT0 是独立按键**，按下直接把 BOOT0 接到 SYS_3.3V（上拉电阻 R3 是 NC 没装）。
   芯片在 1ms 复位循环里救不回来时：**按住 BOOT0 → 按一下 RESET → 烧录 → 松开 BOOT0 → 再按 RESET**。
3. **COM 口是 CH340X，接 USART1（PA9/PA10）**，不是 DAPLink 的虚拟串口。
   自动下载电路里的 `D2` 和 BOOT0 上拉 `R3` 都是 NC，所以只能手动按键。

## logs/

- `ocd_retry.log` —— 06_Watch_dog 救砖过程中反复尝试 SWD 连接的 openocd 原始输出，
  可以看到 `Error: Error connecting DP: cannot read IDR`（芯片复位循环时连不上调试口）。

## 相关工程内还留着的备份

这些 `.bak` 放在各自工程里、和原文件同级，方便随时回滚，没有一起归档：

- `04_USART\test\CM7\Core\Src\main.c.bak`
- `04_USART\test\Makefile\CM7\stm32h747xx_flash_CM7.ld.bak`
- `06_Watch_dog\test\CM7\Core\Src\wwdg.c.bak`
- `07_RTC\test\Makefile\CM7\Makefile.bak`
- `07_RTC\test\Makefile\CM4\Makefile.bak`
