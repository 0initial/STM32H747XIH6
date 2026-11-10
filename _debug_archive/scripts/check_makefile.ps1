param(
    [string]$Makefile = 'Makefile\CM7\Makefile'
)
# 检查 Makefile 的 C_SOURCES 里每个 .c 是否真的存在
$dir  = Split-Path $Makefile
$text = Get-Content $Makefile -Raw
$m = [regex]::Match($text, '(?s)C_SOURCES\s*=(.*?)# ASM sources')
if (-not $m.Success) { Write-Host "没找到 C_SOURCES 段"; exit 1 }
$srcs = [regex]::Matches($m.Groups[1].Value, '\.\./[^\s\\]+\.c') | ForEach-Object { $_.Value }
$missing = @()
foreach ($s in $srcs) {
    if (-not (Test-Path (Join-Path $dir $s))) { $missing += $s }
}
Write-Host ("共 {0} 个源文件，缺失 {1} 个" -f $srcs.Count, $missing.Count)
if ($missing.Count) {
    Write-Host "下面这些行要删掉："
    foreach ($s in $missing) {
        $ln = (Select-String -Path $Makefile -SimpleMatch $s).LineNumber
        Write-Host ("  第 {0} 行: {1}" -f $ln, $s)
    }
} else {
    Write-Host "全部存在，Makefile 和源码一致。"
}
