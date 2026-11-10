$ErrorActionPreference = 'Stop'
$f = 'D:\01_Study_Code\Code\STM32H747XIH6\06_Watch_dog\test\CM7\Core\Src\wwdg.c'
$enc = [System.Text.Encoding]::GetEncoding('GB18030')
Copy-Item $f "$f.bak" -Force
$t = [System.IO.File]::ReadAllText($f, $enc)

$old = @(
  '  hwwdg1.Init.Prescaler = WWDG_PRESCALER_32;',
  '  hwwdg1.Init.Window = 64;',
  '  hwwdg1.Init.Counter = 64;'
) -join "`r`n"

$new = @(
  '  /* PCLK3 = 120 MHz, prescaler 128 -> 1 tick = 4096*128/120MHz = 4.369 ms.',
  '     Counter MUST be 0x41..0x7F: 0x40 underflows to 0x3F in ONE tick and resets at once.',
  '     0x7F -> 64 ticks -> max 279.6 ms; window 0x68 -> refresh allowed after 23 ticks = 100.5 ms. */',
  '  hwwdg1.Init.Prescaler = WWDG_PRESCALER_128;',
  '  hwwdg1.Init.Window = 104;  /* 0x68: too early a refresh also resets */',
  '  hwwdg1.Init.Counter = 127; /* 0x7F */'
) -join "`r`n"

if (([regex]::Matches($t, [regex]::Escape($old))).Count -ne 1) { throw 'WWDG init block not found' }
$t = $t.Replace($old, $new)

$marker = '  /* USER CODE BEGIN WWDG1_Init 2 */'
if (([regex]::Matches($t, [regex]::Escape($marker))).Count -ne 1) { throw 'USER CODE 2 marker not found' }
$t = $t.Replace($marker, $marker + "`r`n" + '  __HAL_DBGMCU_FREEZE_WWDG1(); /* freeze WWDG when the core is halted by the debugger */')

[System.IO.File]::WriteAllText($f, $t, $enc)
Write-Host 'wwdg.c patched'

$gcc = 'D:\Tools\makefile\xpack-arm-none-eabi-gcc-13.3.1-1.1-win32-x64\xpack-arm-none-eabi-gcc-13.3.1-1.1\bin'
$make = 'D:\Tools\make\make-4.4.1-without-guile-w32-bin\bin\make.exe'
Push-Location 'D:\01_Study_Code\Code\STM32H747XIH6\06_Watch_dog\test\Makefile\CM7'
try {
  & $make "GCC_PATH=$gcc" 2>&1 | Select-Object -Last 8 | ForEach-Object { $_ }
  if ($LASTEXITCODE -ne 0) { throw "make failed: $LASTEXITCODE" }
} finally { Pop-Location }
