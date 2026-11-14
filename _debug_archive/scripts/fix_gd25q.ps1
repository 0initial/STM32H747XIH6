$ErrorActionPreference = 'Stop'
$root = 'D:\01_Study_Code\Code\STM32H747XIH6\09_QSPI\test'
$h  = Join-Path $root 'BSP\NORFLASH\norflash.h'
$c  = Join-Path $root 'BSP\NORFLASH\norflash.c'
$enc = [System.Text.Encoding]::GetEncoding('GB18030')
$NL = "`r`n"

Copy-Item $h "$h.bak" -Force
Copy-Item $c "$c.bak" -Force

# ---------- norflash.h : 增加 GD25Q 系列宏 ----------
$t = [System.IO.File]::ReadAllText($h, $enc)
$old = '#define NM25Q128    0x5217U'
if (([regex]::Matches($t, [regex]::Escape($old))).Count -ne 1) { throw 'hdr anchor not unique' }
$new = $old + $NL +
       '#define GD25Q64     0xC816U' + $NL +
       '#define GD25Q128    0xC817U' + $NL +
       '#define GD25Q256    0xC818U   /* 板载实际芯片：GD25Q256，JEDEC = C8 40 19 */'
$t = $t.Replace($old, $new)
[System.IO.File]::WriteAllText($h, $t, $enc)
Write-Host 'norflash.h patched'

# ---------- norflash.c ----------
$t = [System.IO.File]::ReadAllText($c, $enc)

# 1) JEDEC -> type
$old = @(
  '    if (((jedec >> 16) & 0xFFU) != 0xEFU)',
  '    {',
  '        return 0;',
  '    }',
  '',
  '    switch (jedec & 0xFFU)',
  '    {',
  '        case 0x19U: return W25Q256;'
) -join $NL
$new = @(
  '    uint8_t mf  = (uint8_t)((jedec >> 16) & 0xFFU);',
  '    uint8_t cap = (uint8_t)(jedec & 0xFFU);',
  '',
  '    /* 兆易创新 GD25Q 系列（板载实际为 GD25Q256，JEDEC = C8 40 19） */',
  '    if (mf == 0xC8U)',
  '    {',
  '        switch (cap)',
  '        {',
  '            case 0x19U: return GD25Q256;',
  '            case 0x18U: return GD25Q128;',
  '            case 0x17U: return GD25Q64;',
  '            default:    return 0;',
  '        }',
  '    }',
  '',
  '    /* 华邦 W25Q 系列 */',
  '    if (mf != 0xEFU)',
  '    {',
  '        return 0;',
  '    }',
  '',
  '    switch (cap)',
  '    {',
  '        case 0x19U: return W25Q256;'
) -join $NL
if (([regex]::Matches($t, [regex]::Escape($old))).Count -ne 1) { throw 'jedec block not unique' }
$t = $t.Replace($old, $new)

# 2) resolve_chip_id 接受 GD
$old = '    if (id90 == W25Q128 || id90 == W25Q256 || id90 == BY25Q128 || id90 == BY25Q256)'
if (([regex]::Matches($t, [regex]::Escape($old))).Count -ne 1) { throw 'resolve not unique' }
$t = $t.Replace($old, '    if (id90 == W25Q128 || id90 == W25Q256 || id90 == BY25Q128 ||' + $NL +
                      '        id90 == BY25Q256 || id90 == GD25Q128 || id90 == GD25Q256)')

# 3) 4 字节地址模式：把 GD25Q256 也算进 256Mbit 分支，SR3 那段只对华邦做
$old = @(
  '    if (g_norflash_type == W25Q256 || g_norflash_type == BY25Q256)',
  '    {',
  '        s_norflash_addrw = 3U;',
  '        temp = norflash_read_sr(3);',
  '',
  '        if ((temp & 0x01U) == 0U)',
  '        {',
  '            norflash_write_enable();',
  '            temp |= (uint8_t)(1U << 1);',
  '            norflash_write_sr(3, temp);',
  '            HAL_Delay(20);',
  '        }',
  '',
  '        norflash_write_enable();'
) -join $NL
$new = @(
  '    if (g_norflash_type == W25Q256 || g_norflash_type == BY25Q256 ||',
  '        g_norflash_type == GD25Q256)',
  '    {',
  '        s_norflash_addrw = 3U;',
  '',
  '        /* SR3 的 4 字节地址位是华邦特有的，GD25Q 用 0xB7 指令即可，别乱写 SR3 */',
  '        if (g_norflash_type != GD25Q256)',
  '        {',
  '            temp = norflash_read_sr(3);',
  '',
  '            if ((temp & 0x01U) == 0U)',
  '            {',
  '                norflash_write_enable();',
  '                temp |= (uint8_t)(1U << 1);',
  '                norflash_write_sr(3, temp);',
  '                HAL_Delay(20);',
  '            }',
  '        }',
  '',
  '        norflash_write_enable();'
) -join $NL
if (([regex]::Matches($t, [regex]::Escape($old))).Count -ne 1) { throw '256Mbit block not unique' }
$t = $t.Replace($old, $new)

# 4) 0x90 读 ID 后设置地址宽度
$old = @(
  '    if (deviceid == W25Q256 || deviceid == BY25Q256)',
  '    {',
  '        s_norflash_addrw = 3U;',
  '    }'
) -join $NL
if (([regex]::Matches($t, [regex]::Escape($old))).Count -ne 1) { throw 'read_id block not unique' }
$newTxt = @(
  '    if (deviceid == W25Q256 || deviceid == BY25Q256 || deviceid == GD25Q256)',
  '    {',
  '        s_norflash_addrw = 3U;',
  '    }'
) -join $NL
$t = $t.Replace($old, $newTxt)

[System.IO.File]::WriteAllText($c, $t, $enc)
Write-Host 'norflash.c patched'
