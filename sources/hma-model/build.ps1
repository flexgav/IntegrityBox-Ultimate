# Build the standalone reader with no dependency on a specific HMA release.
[CmdletBinding()]
param([string]$Jdk, [string]$AndroidJar, [string]$R8Jar)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
if (-not $AndroidJar) { $AndroidJar = Join-Path $repo 'dex-buildkit\tools\android.jar' }
if (-not $R8Jar) { $R8Jar = Join-Path $repo 'dex-buildkit\tools\r8.jar' }
$javac = if ($Jdk) { Join-Path $Jdk 'bin\javac.exe' } else { (Get-Command javac -ErrorAction Stop).Source }
$java = Join-Path (Split-Path $javac -Parent) 'java.exe'
$work = Join-Path ([IO.Path]::GetTempPath()) ('ib-hma-model-' + [guid]::NewGuid().ToString('N'))
$classes = Join-Path $work 'classes'
$dex = Join-Path $work 'dex'
try {
    New-Item -ItemType Directory -Path $classes, $dex -Force | Out-Null
    & $javac -source 8 -target 8 -Xlint:-options -encoding UTF-8 -bootclasspath $AndroidJar -d $classes (Join-Path $PSScriptRoot 'src\com\flexgav\ibu\HmaModel.java')
    if ($LASTEXITCODE -ne 0) { throw "HMA model reader: javac failed ($LASTEXITCODE)" }
    $files = @(Get-ChildItem $classes -Recurse -Filter '*.class' | ForEach-Object { $_.FullName })
    & $java -cp $R8Jar com.android.tools.r8.D8 --min-api 28 --lib $AndroidJar --output $dex @files
    if ($LASTEXITCODE -ne 0) { throw "HMA model reader: D8 failed ($LASTEXITCODE)" }
    $target = Join-Path $repo 'IntegrityBox-Ultimate-Clnt\common_scripts\hma_api\model.dex'
    Copy-Item (Join-Path $dex 'classes.dex') $target -Force
    Write-Host "Built $target"
} finally {
    Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
}
