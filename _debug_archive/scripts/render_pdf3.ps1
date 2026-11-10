$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Runtime.WindowsRuntime | Out-Null

$asTaskGeneric = ([System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
    $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and
    $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1' })[0]
$asTaskAction = ([System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
    $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and
    $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncAction' })[0]

function AwaitOp($op, $type) {
    $t = $asTaskGeneric.MakeGenericMethod($type).Invoke($null, @($op))
    try { $t.Wait(-1) | Out-Null } catch { throw $t.Exception.InnerException.Message }
    $t.Result
}
function AwaitAction($act) {
    $t = $asTaskAction.Invoke($null, @($act))
    try { $t.Wait(-1) | Out-Null } catch { throw $t.Exception.InnerException.Message }
}

[Windows.Data.Pdf.PdfDocument, Windows.Data.Pdf, ContentType = WindowsRuntime] | Out-Null
[Windows.Storage.Streams.InMemoryRandomAccessStream, Windows.Storage.Streams, ContentType = WindowsRuntime] | Out-Null
[Windows.Data.Pdf.PdfPageRenderOptions, Windows.Data.Pdf, ContentType = WindowsRuntime] | Out-Null

$pdf = 'C:\Users\31414\Documents\deepseek-harness\default-workspace\sch.pdf'
$outDir = 'C:\Users\31414\Documents\deepseek-harness\default-workspace\sch_png'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$bytes = [System.IO.File]::ReadAllBytes($pdf)
Write-Host "pdf bytes: $($bytes.Length)"

$inStream = New-Object Windows.Storage.Streams.InMemoryRandomAccessStream
$writer = New-Object Windows.Storage.Streams.DataWriter($inStream.GetOutputStreamAt(0))
$writer.WriteBytes($bytes)
AwaitOp ($writer.StoreAsync()) ([uint32]) | Out-Null
AwaitOp ($writer.FlushAsync()) ([bool]) | Out-Null
$writer.DetachStream() | Out-Null
$inStream.Seek(0)

try {
    $doc = AwaitOp ([Windows.Data.Pdf.PdfDocument]::LoadFromStreamAsync($inStream)) ([Windows.Data.Pdf.PdfDocument])
} catch {
    Write-Host "LoadFromStreamAsync failed: $_"
    exit 1
}
Write-Host "pages: $($doc.PageCount)"

$opts = New-Object Windows.Data.Pdf.PdfPageRenderOptions
$opts.DestinationWidth = [uint32]2600

for ($i = 0; $i -lt $doc.PageCount; $i++) {
    $page = $doc.GetPage([uint32]$i)
    $stream = New-Object Windows.Storage.Streams.InMemoryRandomAccessStream
    AwaitAction ($page.RenderToStreamAsync($stream, $opts))
    $stream.Seek(0)
    $size = [int]$stream.Size
    $reader = New-Object Windows.Storage.Streams.DataReader($stream.GetInputStreamAt(0))
    AwaitOp ($reader.LoadAsync([uint32]$size)) ([uint32]) | Out-Null
    $buf = New-Object byte[] $size
    $reader.ReadBytes($buf)
    $dst = Join-Path $outDir ("page{0}.png" -f ($i + 1))
    [System.IO.File]::WriteAllBytes($dst, $buf)
    Write-Host "wrote $dst ($size bytes)"
    $reader.Dispose(); $stream.Dispose()
}
