[CmdletBinding()]
param(
    [string]$Port = '',
    [ValidateSet('lcd-7', 'lcd-8', 'lcd-10-1')]
    [string]$Product,
    [switch]$ListOnly,
    [switch]$SelfTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Script:Repository = 'waveshareteam/ESP32-P4-WIFI6-Touch-LCD-X'
$Script:Workflow = 'esp-idf-examples.yml'
$Script:ExampleIdfVersion = 'v6.0.2'
$Script:MaintainedIdfVersion = 'v5.5.5'
$Script:SchemaVersion = 2
$Script:StateSchemaVersion = 3
$Script:FlashSizeBytes = 32MB
$Script:ExpectedExamples = @(
    'examples/esp-idf/07_Displaycolorbar',
    'examples/esp-idf/08_lvgl_demo_v9',
    'examples/esp-idf/09_video_lcd_display',
    'examples/esp-idf/10_mp4_player',
    'examples/esp-idf/11_esp_brookesia_phone',
    'examples/esp-idf/12_usb_extend_screen'
)
$Script:RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))

function Get-FileSha256([string]$Path) {
    $stream = $null; $algorithm = $null
    try {
        $stream = [System.IO.File]::OpenRead($Path)
        $algorithm = [System.Security.Cryptography.SHA256]::Create()
        return [System.BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '').ToLowerInvariant()
    }
    finally {
        if ($null -ne $stream) { $stream.Dispose() }
        if ($null -ne $algorithm) { $algorithm.Dispose() }
    }
}

function Normalize-ArtifactDigest([string]$Value) {
    if ($Value -notmatch '^sha256:([0-9a-fA-F]{64})$') { throw 'Artifact digest must be a SHA-256 digest.' }
    return $Matches[1].ToLowerInvariant()
}

function Assert-ArtifactZipDigest([string]$ZipPath, [string]$ExpectedDigest) {
    if (-not (Test-Path -LiteralPath $ZipPath -PathType Leaf)) { throw 'Artifact ZIP is missing.' }
    $expected = Normalize-ArtifactDigest $ExpectedDigest
    $actual = Get-FileSha256 $ZipPath
    if ($actual -ne $expected) { throw 'Downloaded artifact ZIP digest does not match the GitHub artifact digest.' }
}

function Assert-True([bool]$Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
function Assert-Rejected([scriptblock]$Action, [string]$Message) { try { & $Action } catch { return }; throw $Message }
function Test-Port([string]$Value) { return $Value -match '^COM[0-9]+$' }

function Get-Definitions {
    $manifestPath = Join-Path $Script:RepoRoot 'config/display-variants.json'
    try { $raw = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json }
    catch { throw "Display variant manifest is invalid: $($_.Exception.Message)" }
    if ($null -eq $raw -or @($raw.PSObject.Properties.Name | Sort-Object) -join ',' -ne 'display_examples,variants') { throw 'Display variant manifest has unexpected top-level fields.' }
    $examples = @($raw.display_examples | ForEach-Object { [string]$_ })
    if (@($examples | Sort-Object -Unique).Count -ne 6 -or (@($examples | Sort-Object) -join '|') -ne (@($Script:ExpectedExamples | Sort-Object) -join '|')) { throw 'Display variant manifest must contain exactly the six supported display examples.' }
    $required = @('slug', 'product', 'label', 'resolution', 'panel', 'kconfig', 'overlay')
    $variants = @()
    foreach ($variant in @($raw.variants)) {
        if ($null -eq $variant -or (@($variant.PSObject.Properties.Name | Sort-Object) -join ',') -ne (@($required | Sort-Object) -join ',')) { throw 'Display variant metadata has unexpected fields.' }
        foreach ($field in $required) { if ([string]::IsNullOrWhiteSpace([string]$variant.$field)) { throw "Display variant metadata is missing $field." } }
        $variants += $variant
    }
    if ($variants.Count -ne 3 -or @($variants.slug | Sort-Object -Unique).Count -ne 3) { throw 'Display variant manifest must contain three uniquely named products.' }
    return [pscustomobject]@{ Examples = $examples; Variants = $variants }
}

function New-Items($Definitions, [string]$SelectedProduct, [string]$FinalSha = '', [string]$Profile = 'rev3_x') {
    if ($Profile -notin @('rev1_3', 'rev3_x')) { throw 'Unknown silicon revision profile.' }
    if ($Profile -ne 'rev3_x') { throw 'CI example and maintained-firmware artifacts are published for ESP32-P4 Rev3.x only.' }
    $variant = @($Definitions.Variants | Where-Object { $_.slug -eq $SelectedProduct })
    if ($variant.Count -ne 1) { throw "Unknown display product: $SelectedProduct" }
    $sha12 = if ($FinalSha -match '^[0-9a-fA-F]{40}$') { $FinalSha.Substring(0, 12).ToLowerInvariant() } else { 'sha-at-runtime' }
    $items = @()
    $index = 1
    foreach ($example in $Definitions.Examples) {
        $name = Split-Path -Leaf $example
        $stem = '{0}-{1}-{2}-{3}-{4}' -f $variant[0].product, $name, $Profile, $Script:ExampleIdfVersion.TrimStart('v'), $sha12
        $items += [pscustomobject]@{ Index = $index; Example = $example; ExampleName = $name; IdfVersion = $Script:ExampleIdfVersion; Variant = $variant[0]; Profile = $Profile; ArtifactKind = 'source-built-example'; Workflow = $Script:Workflow; ArtifactStem = $stem; ArtifactName = '' }
        $index++
    }
    $firmwareStem = '{0}-brookesia-{1}-{2}-{3}' -f $variant[0].product, $Profile, $Script:MaintainedIdfVersion.TrimStart('v'), $sha12
    $items += [pscustomobject]@{ Index = $index; Example = 'firmware/brookesia'; ExampleName = 'brookesia'; IdfVersion = $Script:MaintainedIdfVersion; Variant = $variant[0]; Profile = $Profile; ArtifactKind = 'maintained-product-firmware'; Workflow = 'maintained-firmware.yml'; ArtifactStem = $firmwareStem; ArtifactName = '' }
    return $items
}

function Get-NextProgress([int]$CurrentIndex, [int[]]$Confirmed, [int]$ItemCount) {
    if ($ItemCount -lt 1 -or $CurrentIndex -lt 1 -or $CurrentIndex -gt $ItemCount) { throw 'Progress index is outside the item range.' }
    $nextConfirmed = @($Confirmed + $CurrentIndex | Where-Object { $_ -ge 1 -and $_ -le $ItemCount } | Sort-Object -Unique)
    return [pscustomobject]@{ CurrentIndex = if ($CurrentIndex -eq $ItemCount) { $CurrentIndex } else { $CurrentIndex + 1 }; Confirmed = $nextConfirmed; Completed = ($CurrentIndex -eq $ItemCount) }
}

function Get-StateForIdentity($Saved, [string]$ExpectedProduct, [string]$ExpectedSha, [int]$ItemCount, [string]$DefaultPort, [string]$ExpectedProfile = 'rev3_x', [string]$DetectedRevision = '', [string]$DetectedDeviceId = '') {
    $valid = $null -ne $Saved -and [int]$Saved.SchemaVersion -eq $Script:StateSchemaVersion -and [string]$Saved.Product -eq $ExpectedProduct -and [string]$Saved.FinalSha -eq $ExpectedSha -and [string]$Saved.Profile -eq $ExpectedProfile -and [string]$Saved.SiliconRevision -eq $DetectedRevision -and $Saved.PSObject.Properties['DeviceId'] -and (Normalize-DeviceId ([string]$Saved.DeviceId)) -eq (Normalize-DeviceId $DetectedDeviceId) -and $null -ne $Saved.CurrentIndex -and $null -ne $Saved.Confirmed
    if (-not $valid) { return [pscustomobject]@{ CurrentIndex = 1; Confirmed = @(); Port = $DefaultPort; DeviceId = $DetectedDeviceId } }
    $current = [int]$Saved.CurrentIndex
    if ($current -lt 1 -or $current -gt $ItemCount) { throw 'Saved current index is outside the item range.' }
    $savedPort = if ($Saved.PSObject.Properties['Port'] -and (Test-Port ([string]$Saved.Port))) { ([string]$Saved.Port).Trim().ToUpperInvariant() } else { $DefaultPort }
    return [pscustomobject]@{ CurrentIndex = $current; Confirmed = @($Saved.Confirmed | ForEach-Object { [int]$_ } | Where-Object { $_ -ge 1 -and $_ -le $ItemCount } | Sort-Object -Unique); Port = $savedPort; DeviceId = Normalize-DeviceId ([string]$Saved.DeviceId) }
}

function Resolve-SelectedPort([string]$ExplicitPort, [string]$SavedPort, [string]$DefaultPort) {
    if (-not [string]::IsNullOrWhiteSpace($ExplicitPort)) { if (-not (Test-Port $ExplicitPort)) { throw 'Port must be COM followed by digits, for example COMx.' }; return $ExplicitPort.Trim().ToUpperInvariant() }
    foreach ($candidate in @($SavedPort, $DefaultPort)) { if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Port $candidate)) { return $candidate.Trim().ToUpperInvariant() } }
    throw 'Port must be COM followed by digits, for example COMx.'
}

function Test-PackagePath([string]$PackageRoot, [string]$RelativePath) {
    if ([string]::IsNullOrWhiteSpace($RelativePath) -or [System.IO.Path]::IsPathRooted($RelativePath) -or $RelativePath -match '^(?:[\\/]|[A-Za-z]:)' -or $RelativePath -match '(^|[\\/])\.\.([\\/]|$)') { return $false }
    $root = [System.IO.Path]::GetFullPath($PackageRoot).TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    $candidate = [System.IO.Path]::GetFullPath((Join-Path $PackageRoot $RelativePath))
    return $candidate.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)
}

function Assert-ZipEntryNamesSafe([string[]]$Names) {
    $seen = @{}
    foreach ($name in $Names) {
        if ([string]::IsNullOrWhiteSpace($name) -or $name -match '^(?:[\\/]|[A-Za-z]:|//)' -or $name -match '(^|[\\/])\.\.([\\/]|$)' -or $seen.ContainsKey($name.ToLowerInvariant())) { throw "Unsafe or duplicate ZIP entry: $name" }
        $seen[$name.ToLowerInvariant()] = $true
    }
}

function Expand-SafeZip([string]$ZipPath, [string]$Destination) {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($ZipPath)
    try {
        $entries = @($archive.Entries)
        Assert-ZipEntryNamesSafe @($entries | ForEach-Object { $_.FullName })
        foreach ($entry in $entries) {
            if ($entry.FullName.EndsWith('/')) { continue }
            $target = Join-Path $Destination ($entry.FullName -replace '/', [System.IO.Path]::DirectorySeparatorChar)
            if (-not (Test-PackagePath $Destination $entry.FullName)) { throw "ZIP entry escapes the package: $($entry.FullName)" }
            $parent = Split-Path -Parent $target
            [System.IO.Directory]::CreateDirectory($parent) | Out-Null
            $input = $null; $output = $null
            try { $input = $entry.Open(); $output = [System.IO.File]::Open($target, [System.IO.FileMode]::CreateNew); $input.CopyTo($output) }
            finally { if ($null -ne $output) { $output.Dispose() }; if ($null -ne $input) { $input.Dispose() } }
        }
    }
    finally { $archive.Dispose() }
}

function Get-ChecksumMap([string]$PackageDir) {
    $checksumPath = Join-Path $PackageDir 'SHA256SUMS'
    if (-not (Test-Path -LiteralPath $checksumPath -PathType Leaf)) { throw 'SHA256SUMS is missing.' }
    $checksums = @{}
    foreach ($line in Get-Content -LiteralPath $checksumPath) {
        if ($line -notmatch '^([0-9a-fA-F]{64})  (.+)$') { throw 'SHA256SUMS contains an invalid line.' }
        $path = $Matches[2]
        if (-not (Test-PackagePath $PackageDir $path) -or $checksums.ContainsKey($path)) { throw "SHA256SUMS contains an unsafe or duplicate path: $path" }
        $checksums[$path] = $Matches[1].ToLowerInvariant()
    }
    $packagePrefix = $PackageDir.TrimEnd([char[]]@([char]92, [char]47))
    $expected = @((Get-ChildItem -LiteralPath $PackageDir -Recurse -File | ForEach-Object { $_.FullName.Substring($packagePrefix.Length + 1).Replace([char]92, [char]47) } | Where-Object { $_ -ne 'SHA256SUMS' }) | Sort-Object)
    if ((@($checksums.Keys | Sort-Object) -join '|') -ne ($expected -join '|')) { throw 'SHA256SUMS must cover every package file except itself exactly once.' }
    foreach ($path in $checksums.Keys) { if ((Get-FileSha256 (Join-Path $PackageDir $path)) -ne $checksums[$path]) { throw "SHA256SUMS verification failed: $path" } }
    return $checksums
}

function Get-SafeFlashArguments([string]$PackageDir) {
    $path = Join-Path $PackageDir 'flash_args'
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw 'flash_args is missing.' }
    $tokens = @((Get-Content -LiteralPath $path) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    $optionAliases = @{
        '--flash_mode' = 'flash_mode'; '--flash-mode' = 'flash_mode'
        '--flash_freq' = 'flash_freq'; '--flash-freq' = 'flash_freq'
        '--flash_size' = 'flash_size'; '--flash-size' = 'flash_size'
    }
    $index = 0; $options = @(); $seenOptions = @{}
    while ($index -lt $tokens.Count -and $tokens[$index].StartsWith('--')) {
        $option = [string]$tokens[$index]
        if (-not $optionAliases.ContainsKey($option)) { throw "flash_args has an unsupported option: $option" }
        $normalizedOption = $optionAliases[$option]
        if ($seenOptions.ContainsKey($normalizedOption)) { throw "flash_args repeats an option: $option" }
        if (($index + 1) -ge $tokens.Count) { throw "flash_args option is missing its value: $option" }
        $value = [string]$tokens[$index + 1]
        $valid = ($normalizedOption -eq 'flash_mode' -and $value -in @('qio','qout','dio','dout')) -or ($normalizedOption -eq 'flash_freq' -and $value -match '^(20|26|40|80)m$') -or ($normalizedOption -eq 'flash_size' -and $value -eq '32MB')
        if (-not $valid) { throw "flash_args has an unsafe value for $option." }
        $seenOptions[$normalizedOption] = $true
        $options += $option
        $options += $value
        $index += 2
    }
    if (-not $seenOptions.ContainsKey('flash_size')) { throw 'flash_args must explicitly set --flash_size 32MB.' }
    $pairs = @()
    while ($index -lt $tokens.Count) {
        if (($index + 1) -ge $tokens.Count) { throw 'flash_args has an incomplete offset/path pair.' }
        $offset = [string]$tokens[$index]; $relative = [string]$tokens[$index + 1]
        if ($offset -notmatch '^0x[0-9a-f]+$' -or -not (Test-PackagePath $PackageDir $relative)) { throw 'flash_args has an unsafe offset or path.' }
        $pairs += [pscustomobject]@{ Offset = $offset; Path = $relative }
        $index += 2
    }
    if ($pairs.Count -lt 1) { throw 'flash_args contains no split files.' }
    return [pscustomobject]@{ Options = $options; Pairs = $pairs }
}

function New-EsptoolArguments([string]$SelectedPort, $Plan) {
    if (-not (Test-Port $SelectedPort) -or @($Plan).Count -lt 1) { throw 'Unsafe esptool command contract.' }
    $options = @($Plan[0].WriteFlashOptions)
    $arguments = @('-m', 'esptool', '--port', $SelectedPort, '--chip', 'esp32p4', '--baud', '460800', 'write_flash') + $options
    foreach ($entry in $Plan) {
        if ([string]$entry.Offset -notmatch '^0x[0-9a-f]+$' -or -not (Test-Path -LiteralPath $entry.FullPath -PathType Leaf)) { throw 'Unsafe split file in esptool command contract.' }
        $arguments += $entry.Offset
        $arguments += $entry.FullPath
    }
    return $arguments
}

function Test-PackageManifest([string]$PackageDir, $Item, [string]$FinalSha) {
    $manifestPath = Join-Path $PackageDir 'manifest.json'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'manifest.json is missing.' }
    try { $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json } catch { throw "manifest.json is invalid: $($_.Exception.Message)" }
    $identity = [int]$manifest.schema_version -eq $Script:SchemaVersion -and [int64]$manifest.flash_size_bytes -eq $Script:FlashSizeBytes -and $manifest.artifact_kind -eq $Item.ArtifactKind -and $manifest.factory_firmware -eq $false -and $manifest.framework -eq 'ESP-IDF' -and $manifest.idf_version -eq $Item.IdfVersion -and $manifest.target -eq 'esp32p4' -and $manifest.example -eq $Item.Example -and $manifest.variant -eq $Item.Variant.slug -and $manifest.product -eq $Item.Variant.product -and $manifest.resolution -eq $Item.Variant.resolution -and $manifest.panel -eq $Item.Variant.panel -and $manifest.kconfig -eq $Item.Variant.kconfig -and $manifest.revision_profile -eq $Item.Profile -and $manifest.git_sha -eq $FinalSha -and [int]$manifest.baud -eq 460800
    if (-not $identity) { throw 'Package manifest identity does not match the selected product, example, and final SHA.' }
    if ($null -eq $manifest.revision_bounds -or [string]$manifest.revision_bounds.min -ne $(if ($Item.Profile -eq 'rev1_3') {'1.0'} else {'3.0'}) -or (($Item.Profile -eq 'rev1_3') -and [string]$manifest.revision_bounds.max_exclusive -ne '3.0') -or (($Item.Profile -eq 'rev3_x') -and $null -ne $manifest.revision_bounds.max_exclusive)) { throw 'Package manifest revision bounds are invalid.' }
    $null = Get-ChecksumMap $PackageDir
    $merged = $manifest.merged_image
    if ($null -eq $merged -or [string]$merged.path -ne 'bin/merged-flash.bin' -or $null -eq $merged.size -or [int64]$merged.size -le 0 -or [int64]$merged.size -gt $Script:FlashSizeBytes -or [string]$merged.sha256 -notmatch '^[0-9a-fA-F]{64}$') { throw 'Manifest merged image metadata is unsafe.' }
    $mergedPath = Join-Path $PackageDir ([string]$merged.path)
    if (-not (Test-PackagePath $PackageDir ([string]$merged.path)) -or -not (Test-Path -LiteralPath $mergedPath -PathType Leaf) -or [int64](Get-Item -LiteralPath $mergedPath).Length -ne [int64]$merged.size -or (Get-FileSha256 $mergedPath) -ne ([string]$merged.sha256).ToLowerInvariant()) { throw 'Manifest merged image checksum verification failed.' }
    $split = @{}; $plan = @()
    foreach ($file in @($manifest.files)) {
        $relative = [string]$file.path; $offset = [string]$file.offset; $digest = [string]$file.sha256
        if (-not (Test-PackagePath $PackageDir $relative) -or $digest -notmatch '^[0-9a-fA-F]{64}$' -or $null -eq $file.size -or [int64]$file.size -le 0) { throw 'Manifest file metadata is unsafe.' }
        $fullPath = Join-Path $PackageDir $relative
        if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf) -or [int64](Get-Item -LiteralPath $fullPath).Length -ne [int64]$file.size -or (Get-FileSha256 $fullPath) -ne $digest.ToLowerInvariant()) { throw "Manifest checksum verification failed: $relative" }
        if ($offset -eq 'merged') { continue }
        if ($offset -notmatch '^0x[0-9a-f]+$' -or $split.ContainsKey($offset)) { throw "Manifest has an invalid or duplicate split offset: $offset" }
        $entry = [pscustomobject]@{ Offset = $offset; OffsetValue = [Convert]::ToInt64($offset.Substring(2), 16); Path = $relative; FullPath = $fullPath; Size = [int64](Get-Item -LiteralPath $fullPath).Length }
        if ($entry.OffsetValue + $entry.Size -gt $Script:FlashSizeBytes) { throw "Manifest split range exceeds the documented 32MB NOR flash: $relative" }
        $split[$offset] = $entry; $plan += $entry
    }
    if ($split.Count -lt 1 -or $null -eq $manifest.offsets) { throw 'Manifest has no split flash plan.' }
    $offsetPairs = @($manifest.offsets.PSObject.Properties | ForEach-Object { "$($_.Name)`t$($_.Value)" } | Sort-Object)
    $splitPairs = @($split.Values | ForEach-Object { "$($_.Offset)`t$($_.Path)" } | Sort-Object)
    if (($offsetPairs -join '|') -ne ($splitPairs -join '|')) { throw 'Manifest offsets and split files disagree.' }
    $parsedFlash = Get-SafeFlashArguments $PackageDir
    $flashPairs = $parsedFlash.Pairs
    $flashMap = @{}; foreach ($pair in $flashPairs) { if ($flashMap.ContainsKey($pair.Offset)) { throw 'flash_args repeats a split offset.' }; $flashMap[$pair.Offset] = $pair.Path }
    $flashPairsNormalized = @($flashMap.Keys | ForEach-Object { "$_`t$($flashMap[$_])" } | Sort-Object)
    if (($flashPairsNormalized -join '|') -ne ($splitPairs -join '|')) { throw 'flash_args and manifest split files disagree.' }
    $ordered = @($plan | Sort-Object OffsetValue)
    for ($i = 1; $i -lt $ordered.Count; $i++) { if ($ordered[$i - 1].OffsetValue + $ordered[$i - 1].Size -gt $ordered[$i].OffsetValue) { throw 'Manifest split flash ranges overlap.' } }
    foreach ($entry in $ordered) { $entry | Add-Member -NotePropertyName WriteFlashOptions -NotePropertyValue @($parsedFlash.Options) }
    return $ordered
}

function Resolve-Executable([string]$Name, [string[]]$Fallbacks) {
    $command = Get-Command $Name -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($command -and $command.Source) { return $command.Source }
    foreach ($candidate in $Fallbacks) { if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) { return $candidate } }
    throw "$Name was not found on PATH or in supported locations."
}
function Resolve-Git { return Resolve-Executable 'git' @((Join-Path $env:ProgramFiles 'Git/cmd/git.exe'), 'C:\Git\cmd\git.exe', 'D:\Git\cmd\git.exe') }
function Resolve-Gh { return Resolve-Executable 'gh' @((Join-Path $env:ProgramFiles 'GitHub CLI/gh.exe')) }
function Resolve-PythonWithEsptool {
    $candidates = @(); $command = Get-Command python -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($command -and $command.Source) { $candidates += $command.Source }
    if ($env:IDF_PYTHON_ENV_PATH) { $candidates += Join-Path $env:IDF_PYTHON_ENV_PATH 'Scripts/python.exe' }
    if ($env:USERPROFILE) { $candidates += @(Get-ChildItem -LiteralPath (Join-Path $env:USERPROFILE '.espressif/python_env') -Directory -ErrorAction SilentlyContinue | ForEach-Object { Join-Path $_.FullName 'Scripts/python.exe' }) }
    $candidates += @(Get-ChildItem -Path 'C:\Espressif\python_env\*' -Directory -ErrorAction SilentlyContinue | ForEach-Object { Join-Path $_.FullName 'Scripts/python.exe' })
    $candidates += @(Get-ChildItem -Path 'D:\espressif\python_env\*' -Directory -ErrorAction SilentlyContinue | ForEach-Object { Join-Path $_.FullName 'Scripts/python.exe' })
    if ($env:ProgramFiles) { $candidates += @(Get-ChildItem -Path (Join-Path $env:ProgramFiles 'Python*') -Directory -ErrorAction SilentlyContinue | ForEach-Object { Join-Path $_.FullName 'python.exe' }) }
    if ($env:LOCALAPPDATA) { $candidates += @(Get-ChildItem -Path (Join-Path $env:LOCALAPPDATA 'Programs/Python/Python*') -Directory -ErrorAction SilentlyContinue | ForEach-Object { Join-Path $_.FullName 'python.exe' }) }
    foreach ($candidate in @($candidates | Where-Object { $_ -and (Test-Path -LiteralPath $_ -PathType Leaf) } | Select-Object -Unique)) { & $candidate -c 'import esptool' *> $null; if ($LASTEXITCODE -eq 0) { return $candidate } }
    throw 'No Python interpreter that can import esptool was found.'
}
function Normalize-DeviceId([string]$Value) {
    if ($Value -notmatch '^(?i:[0-9a-f]{2}(?::[0-9a-f]{2}){5})$') { throw 'ESP32-P4 MAC address is missing or invalid; refusing to identify the device.' }
    return $Value.ToLowerInvariant()
}
function Parse-SiliconRevisionOutput([string]$output) {
    if ($output -notmatch '(?i)ESP32[- ]P4') { throw 'The selected port did not identify an ESP32-P4.' }
    $match = [regex]::Match($output, '(?i)(?:revision|rev(?:ision)?\s*:?|chip\s+revision\s*:?)\s*v?(\d+)\.(\d+)')
    if (-not $match.Success) { throw 'ESP32-P4 silicon revision is unknown or unparseable.' }
    $major = [int]$match.Groups[1].Value; $minor = [int]$match.Groups[2].Value
    $mac = [regex]::Match($output, '(?im)^\s*MAC\s*:\s*([0-9a-f]{2}(?::[0-9a-f]{2}){5})\b')
    if (-not $mac.Success) { throw 'ESP32-P4 MAC address is missing from chip_id output; refusing to identify the device.' }
    return [pscustomobject]@{ Dotted = "$major.$minor"; Full = ($major * 100 + $minor); DeviceId = Normalize-DeviceId $mac.Groups[1].Value }
}
function Get-SiliconRevision([string]$PythonExe, [string]$SelectedPort) {
    if (-not (Test-Port $SelectedPort)) { throw 'Port must be COM followed by digits before probing silicon revision.' }
    $output = (& $PythonExe -m esptool --chip esp32p4 --port $SelectedPort chip_id 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) { $output = (& $PythonExe -m esptool --chip esp32p4 --port $SelectedPort chip-id 2>&1 | Out-String) }
    if ($LASTEXITCODE -ne 0) { throw 'Unable to probe the selected port as ESP32-P4 with non-destructive chip_id.' }
    return Parse-SiliconRevisionOutput $output
}
function Get-RevisionProfile($Revision) {
    if ($null -eq $Revision -or [int]$Revision.Full -lt 0) { throw 'ESP32-P4 silicon revision is unknown or unparseable.' }
    if ([int]$Revision.Full -lt 300) { return 'rev1_3' }
    return 'rev3_x'
}
function Assert-RevisionProfile([string]$Profile, $Revision) {
    $expected = Get-RevisionProfile $Revision
    if ($Profile -ne $expected) { throw "Artifact profile $Profile is incompatible with detected ESP32-P4 silicon revision $($Revision.Dotted)." }
}
function Assert-SameSiliconIdentity($Expected, $Observed) {
    $expectedDeviceId = if ($null -ne $Expected -and $Expected.PSObject.Properties['DeviceId']) { Normalize-DeviceId ([string]$Expected.DeviceId) } else { '' }
    $observedDeviceId = if ($null -ne $Observed -and $Observed.PSObject.Properties['DeviceId']) { Normalize-DeviceId ([string]$Observed.DeviceId) } else { '' }
    if ($null -eq $Expected -or $null -eq $Observed -or [string]$Expected.Dotted -ne [string]$Observed.Dotted -or [int]$Expected.Full -ne [int]$Observed.Full -or [string]::IsNullOrWhiteSpace($expectedDeviceId) -or [string]::IsNullOrWhiteSpace($observedDeviceId) -or $expectedDeviceId -ne $observedDeviceId) { throw 'Selected port silicon identity changed; refusing to flash.' }
}
function Resolve-FinalSha([string]$GitExe) { $sha = (& $GitExe -C $Script:RepoRoot rev-parse HEAD 2>&1 | Out-String).Trim(); if ($LASTEXITCODE -ne 0 -or $sha -notmatch '^[0-9a-fA-F]{40}$') { throw 'Unable to resolve a full local HEAD SHA.' }; return $sha.ToLowerInvariant() }
function Assert-LocalGitReady([string]$GitExe) {
    $status = (& $GitExe -C $Script:RepoRoot status --porcelain=v1 --untracked-files=all 2>&1 | Out-String); if ($LASTEXITCODE -ne 0 -or -not [string]::IsNullOrWhiteSpace($status)) { throw 'Refusing to continue: the working tree must be clean.' }
    $branch = (& $GitExe -C $Script:RepoRoot symbolic-ref --quiet --short HEAD 2>&1 | Out-String).Trim(); if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($branch)) { throw 'Refusing to continue: check out a non-detached branch.' }
    $upstream = (& $GitExe -C $Script:RepoRoot rev-parse --abbrev-ref '@{upstream}' 2>&1 | Out-String).Trim(); if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($upstream)) { throw 'Refusing to continue: the current branch must have an upstream.' }
    return $branch
}
function Assert-ReadyPullRequest([string]$GhExe, [string]$Branch, [string]$FinalSha) {
    $raw = (& $GhExe pr list --repo $Script:Repository --head $Branch --state open --limit 2 --json number,state,isDraft,headRefName,headRefOid 2>&1 | Out-String); if ($LASTEXITCODE -ne 0) { throw 'Unable to query the open pull request.' }
    $prs = @($raw | ConvertFrom-Json); if ($prs.Count -ne 1 -or [string]$prs[0].state -ine 'OPEN' -or [bool]$prs[0].isDraft -or [string]$prs[0].headRefName -ne $Branch -or [string]$prs[0].headRefOid -ine $FinalSha) { throw 'Refusing to continue: exactly one non-draft open PR for this branch must point at local HEAD.' }
}
function Resolve-ArtifactRun([string]$GhExe, [string]$FinalSha, $Items) {
    foreach ($group in @($Items | Group-Object Workflow)) {
        $raw = (& $GhExe run list --repo $Script:Repository --workflow $group.Name --commit $FinalSha --status success --limit 50 --json databaseId,headSha,createdAt 2>&1 | Out-String); if ($LASTEXITCODE -ne 0) { throw "Unable to list successful $($group.Name) workflow runs." }
        $found = $false
        foreach ($run in @($raw | ConvertFrom-Json | Where-Object { [string]$_.headSha -ieq $FinalSha } | Sort-Object createdAt -Descending)) {
            $artifactsRaw = (& $GhExe api "repos/$Script:Repository/actions/runs/$($run.databaseId)/artifacts?per_page=100" 2>&1 | Out-String); if ($LASTEXITCODE -ne 0) { throw "Unable to list artifacts for run $($run.databaseId)." }
            $artifacts = @((($artifactsRaw | ConvertFrom-Json).artifacts)); $resolved = @{}; $complete = $true
            foreach ($item in @($group.Group)) {
                $matches = @($artifacts | Where-Object { ([string]$_.name -eq $item.ArtifactStem -or [string]$_.name -eq ($item.ArtifactStem + '.zip')) })
                if ($matches.Count -ne 1 -or [int64]$matches[0].size_in_bytes -le 0 -or [bool]$matches[0].expired -or [string]$matches[0].digest -notmatch '^sha256:[0-9a-fA-F]{64}$') { $complete = $false; break }
                $resolved[$item.Index] = $matches[0]
            }
            if ($complete -and $resolved.Count -eq @($group.Group).Count) { foreach ($item in @($group.Group)) { $item.ArtifactName = [string]$resolved[$item.Index].name; $item | Add-Member -NotePropertyName ArtifactId -NotePropertyValue ([int64]$resolved[$item.Index].id) -Force; $item | Add-Member -NotePropertyName ArtifactDigest -NotePropertyValue ([string]$resolved[$item.Index].digest) -Force; $item | Add-Member -NotePropertyName RunId -NotePropertyValue ([string]$run.databaseId) -Force }; $found = $true; break }
        }
        if (-not $found) { throw "No successful final-SHA $($group.Name) run contains every required profile/product artifact." }
    }
    return $FinalSha
}
function Resolve-DefaultPort {
    $ports = @(Get-CimInstance Win32_PnPEntity -ErrorAction SilentlyContinue | Where-Object { $_.PNPDeviceID -match 'VID_303A' -and $_.Name -match '\(COM[0-9]+\)' } | ForEach-Object { [regex]::Match($_.Name, '\((COM[0-9]+)\)').Groups[1].Value } | Sort-Object -Unique)
    if ($ports.Count -ne 1) { throw 'Unable to identify exactly one Espressif USB serial port; pass -Port COMx.' }; return $ports[0]
}
function Get-StateRoot { return Join-Path $env:LOCALAPPDATA 'Waveshare/ESP32-P4-WIFI6-Touch-LCD-X/ci-firmware' }
function Read-State([string]$ProductName, [string]$FinalSha, [int]$ItemCount, [string]$Profile, [string]$SiliconRevision, [string]$DeviceId) { $path = Join-Path (Get-StateRoot) 'state-v3.json'; $saved = if (Test-Path -LiteralPath $path) { Get-Content -LiteralPath $path -Raw | ConvertFrom-Json } else { $null }; return Get-StateForIdentity $saved $ProductName $FinalSha $ItemCount '' $Profile $SiliconRevision $DeviceId }
function Save-State([int]$CurrentIndex, [int[]]$Confirmed, [string]$SavedPort, [string]$ProductName, [string]$FinalSha, [string]$Profile, [string]$SiliconRevision, [string]$DeviceId) { $root = Get-StateRoot; [System.IO.Directory]::CreateDirectory($root) | Out-Null; [pscustomobject]@{ SchemaVersion = $Script:StateSchemaVersion; Product = $ProductName; FinalSha = $FinalSha; Profile = $Profile; SiliconRevision = $SiliconRevision; DeviceId = (Normalize-DeviceId $DeviceId); CurrentIndex = $CurrentIndex; Confirmed = @($Confirmed | Sort-Object -Unique); Port = $SavedPort; UpdatedAtUtc = [DateTime]::UtcNow.ToString('o') } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root 'state-v3.json') -Encoding UTF8 }
function New-RunPaths { $root = Get-StateRoot; $stamp = [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffZ'); $download = Join-Path $root "downloads/$stamp"; $logs = Join-Path $root 'logs'; [System.IO.Directory]::CreateDirectory($download) | Out-Null; [System.IO.Directory]::CreateDirectory($logs) | Out-Null; return [pscustomobject]@{ DownloadDir = $download; LogPath = (Join-Path $logs "$stamp.log") } }
function Add-RunLog([string]$Path, [string]$Text) { Add-Content -LiteralPath $Path -Value $Text -Encoding UTF8 }
function Get-ArtifactTransportZip([string]$GhExe, [int64]$ArtifactId, [string]$Destination) {
    if ($ArtifactId -le 0) { throw 'Artifact ID must be a positive integer.' }
    if (Test-Path -LiteralPath $Destination) { throw 'Artifact transport ZIP destination already exists.' }
    $stderrPath = $Destination + '.stderr'
    if (Test-Path -LiteralPath $stderrPath) { throw 'Artifact transport stderr destination already exists.' }
    $endpoint = "repos/$Script:Repository/actions/artifacts/$ArtifactId/zip"
    $process = Start-Process -FilePath $GhExe -ArgumentList @('api', $endpoint) -NoNewWindow -Wait -PassThru -RedirectStandardOutput $Destination -RedirectStandardError $stderrPath
    if ($process.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $Destination -PathType Leaf) -or [int64](Get-Item -LiteralPath $Destination).Length -le 0) { throw "Artifact raw ZIP download failed with exit code $($process.ExitCode)." }
    Remove-Item -LiteralPath $stderrPath -Force
    return $Destination
}
function Invoke-CurrentFlash($Item, [string]$SelectedPort, [string]$RunId, [string]$GhExe, [string]$PythonExe, [string]$FinalSha) {
    $paths = New-RunPaths; Add-RunLog $paths.LogPath "final_sha=$FinalSha run_id=$RunId artifact_id=$($Item.ArtifactId) artifact=$($Item.ArtifactName) product=$($Item.Variant.slug) example=$($Item.Example) port=$SelectedPort"
    $transportZip = Get-ArtifactTransportZip $GhExe ([int64]$Item.ArtifactId) (Join-Path $paths.DownloadDir 'artifact-transport.zip')
    Assert-ArtifactZipDigest $transportZip $Item.ArtifactDigest
    $packageDir = Join-Path $paths.DownloadDir 'package'; [System.IO.Directory]::CreateDirectory($packageDir) | Out-Null; Expand-SafeZip $transportZip $packageDir
    $manifests = @(Get-ChildItem -LiteralPath $packageDir -Recurse -File -Filter 'manifest.json'); if ($manifests.Count -ne 1) { throw 'Downloaded ZIP must contain exactly one manifest.json.' }
    $plan = Test-PackageManifest $manifests[0].DirectoryName $Item $FinalSha
    $arguments = New-EsptoolArguments $SelectedPort $plan
    $output = (& $PythonExe @arguments 2>&1 | Out-String); $code = $LASTEXITCODE; Add-RunLog $paths.LogPath $output
    return [pscustomobject]@{ Success = ($code -eq 0 -and $output.Contains('Hash of data verified')); Output = $output; LogPath = $paths.LogPath }
}

function Invoke-SelfTest {
    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $definitions = Get-Definitions
    foreach ($variant in $definitions.Variants) {
        $generated = @(New-Items $definitions $variant.slug ('a' * 40) 'rev3_x')
        Assert-True ($generated.Count -eq 7) 'SelfTest product/example generation failed.'
        Assert-True ($generated[0].IdfVersion -eq 'v6.0.2' -and $generated[0].ArtifactStem -match '-rev3_x-6\.0\.2-[a-f0-9]{12}$') 'SelfTest example artifact identity failed.'
        Assert-True ($generated[-1].Example -eq 'firmware/brookesia' -and $generated[-1].IdfVersion -eq 'v5.5.5' -and $generated[-1].ArtifactStem -match '-brookesia-rev3_x-5\.5\.5-[a-f0-9]{12}$') 'SelfTest maintained artifact identity failed.'
        Assert-Rejected { New-Items $definitions $variant.slug ('a' * 40) 'rev1_3' } 'SelfTest accepted an unpublished Rev1.3 artifact set.'
    }
    $next = Get-NextProgress 1 @() 7; Assert-True ($next.CurrentIndex -eq 2 -and @($next.Confirmed).Count -eq 1) 'SelfTest progress advance failed.'
    $rev13 = Parse-SiliconRevisionOutput "Chip is ESP32-P4 (revision v1.3)`nMAC: AA:BB:CC:DD:EE:FF"; Assert-True ($rev13.Dotted -eq '1.3' -and $rev13.DeviceId -eq 'aa:bb:cc:dd:ee:ff' -and (Get-RevisionProfile $rev13) -eq 'rev1_3') 'SelfTest rev1_3 parsing/gating failed.'
    $rev30 = Parse-SiliconRevisionOutput "ESP32-P4 chip revision: 3.0`nMAC: 00:11:22:33:44:55"; Assert-True ((Get-RevisionProfile $rev30) -eq 'rev3_x') 'SelfTest rev3_x parsing/gating failed.'
    Assert-Rejected { Parse-SiliconRevisionOutput 'Chip is ESP32-C6 revision v0.1' } 'SelfTest non-P4 probe was accepted.'; Assert-Rejected { Parse-SiliconRevisionOutput 'Chip is ESP32-P4 (revision v1.3)' } 'SelfTest probe without MAC was accepted.'; Assert-Rejected { Parse-SiliconRevisionOutput "Chip is ESP32-P4`nMAC: AA:BB:CC:DD:EE:FF" } 'SelfTest unparseable revision was accepted.'; Assert-Rejected { Assert-RevisionProfile 'rev3_x' $rev13 } 'SelfTest cross-profile artifact was accepted.'; Assert-SameSiliconIdentity $rev13 ([pscustomobject]@{ Dotted='1.3'; Full=103; DeviceId='AA:BB:CC:DD:EE:FF' }); Assert-Rejected { Assert-SameSiliconIdentity $rev13 $rev30 } 'SelfTest silicon revision change was accepted.'; Assert-Rejected { Assert-SameSiliconIdentity $rev13 ([pscustomobject]@{ Dotted='1.3'; Full=103; DeviceId='00:11:22:33:44:55' }) } 'SelfTest silicon MAC change was accepted.'; Assert-Rejected { Assert-SameSiliconIdentity $rev13 ([pscustomobject]@{ Dotted='1.3'; Full=103 }) } 'SelfTest silicon identity without MAC was accepted.'
    $resetSha = Get-StateForIdentity ([pscustomobject]@{ SchemaVersion=3; Product='lcd-7'; FinalSha='old'; Profile='rev1_3'; SiliconRevision='1.0'; DeviceId='aa:bb:cc:dd:ee:ff'; CurrentIndex=3; Confirmed=@(1,2) }) 'lcd-7' ('a' * 40) 6 '' 'rev1_3' '1.0' 'aa:bb:cc:dd:ee:ff'; Assert-True ($resetSha.CurrentIndex -eq 1) 'SelfTest SHA reset failed.'
    $resetProduct = Get-StateForIdentity ([pscustomobject]@{ SchemaVersion=3; Product='lcd-7'; FinalSha=('a' * 40); Profile='rev1_3'; SiliconRevision='1.0'; DeviceId='aa:bb:cc:dd:ee:ff'; CurrentIndex=3; Confirmed=@(1,2) }) 'lcd-8' ('a' * 40) 6 '' 'rev1_3' '1.0' 'aa:bb:cc:dd:ee:ff'; Assert-True ($resetProduct.CurrentIndex -eq 1) 'SelfTest product reset failed.'
    $savedPort = Get-StateForIdentity ([pscustomobject]@{ SchemaVersion=3; Product='lcd-7'; FinalSha=('a' * 40); Profile='rev1_3'; SiliconRevision='1.0'; DeviceId='AA:BB:CC:DD:EE:FF'; CurrentIndex=3; Confirmed=@(1,2); Port='com17' }) 'lcd-7' ('a' * 40) 6 '' 'rev1_3' '1.0' 'aa:bb:cc:dd:ee:ff'; Assert-True ($savedPort.Port -eq 'COM17' -and $savedPort.DeviceId -eq 'aa:bb:cc:dd:ee:ff') 'SelfTest saved port/device restore failed.'
    Assert-True ((Resolve-SelectedPort 'COM18' 'COM17' 'COM19') -eq 'COM18') 'SelfTest explicit port override failed.'; Assert-True ((Resolve-SelectedPort '' 'COM17' 'COM19') -eq 'COM17') 'SelfTest saved port priority failed.'
    Assert-True (-not (Test-PackagePath ([System.IO.Path]::GetTempPath()) '../escape.bin')) 'SelfTest path escape failed.'; Assert-True (-not (Test-PackagePath ([System.IO.Path]::GetTempPath()) 'C:\rooted.bin')) 'SelfTest rooted path failed.'
    try { Assert-ZipEntryNamesSafe @('bin/a.bin','BIN/a.bin'); throw 'SelfTest duplicate ZIP entries were accepted.' } catch { if ($_.Exception.Message -eq 'SelfTest duplicate ZIP entries were accepted.') { throw } }
    $temp = Join-Path ([System.IO.Path]::GetTempPath()) ('p4-flasher-selftest-' + [Guid]::NewGuid().ToString('N')); [System.IO.Directory]::CreateDirectory((Join-Path $temp 'bin')) | Out-Null
    try {
        [System.IO.File]::WriteAllBytes((Join-Path $temp 'bin/boot.bin'), [byte[]](1,2,3)); [System.IO.File]::WriteAllBytes((Join-Path $temp 'bin/app.bin'), [byte[]](4,5,6,7)); [System.IO.File]::WriteAllBytes((Join-Path $temp 'bin/merged-flash.bin'), [byte[]](8)); Set-Content -LiteralPath (Join-Path $temp 'flash_args') -Value @('--flash-mode','dio','--flash-size','32MB','0x1000','bin/boot.bin','0x10000','bin/app.bin') -Encoding utf8
        $artifactZip = Join-Path $temp 'artifact.zip'; $transportPackage = Join-Path $temp 'transport-package'; Add-Type -AssemblyName System.IO.Compression.FileSystem; $archive = [System.IO.Compression.ZipFile]::Open($artifactZip, [System.IO.Compression.ZipArchiveMode]::Create); try { $entry = $archive.CreateEntry('manifest.json'); $writer = New-Object System.IO.StreamWriter($entry.Open()); try { $writer.Write('{}') } finally { $writer.Dispose() } } finally { $archive.Dispose() }; $artifactItem = [pscustomobject]@{ ArtifactDigest = ('sha256:' + (Get-FileSha256 $artifactZip)) }; Assert-ArtifactZipDigest $artifactZip $artifactItem.ArtifactDigest; Expand-SafeZip $artifactZip $transportPackage; Assert-True (Test-Path -LiteralPath (Join-Path $transportPackage 'manifest.json') -PathType Leaf) 'SelfTest artifact transport ZIP expansion failed.'; Add-Content -LiteralPath $artifactZip -Value 'tamper'; Assert-Rejected { Assert-ArtifactZipDigest $artifactZip $artifactItem.ArtifactDigest } 'SelfTest artifact ZIP digest tamper was accepted.'; Remove-Item -LiteralPath $artifactZip -Force; Remove-Item -LiteralPath $transportPackage -Recurse -Force
        $variant = $definitions.Variants[0]; $files = @(@{offset='0x1000';path='bin/boot.bin';size=3;sha256=(Get-FileSha256 (Join-Path $temp 'bin/boot.bin'))}, @{offset='0x10000';path='bin/app.bin';size=4;sha256=(Get-FileSha256 (Join-Path $temp 'bin/app.bin'))})
        $manifest = @{schema_version=2;flash_size_bytes=33554432;artifact_kind='source-built-example';factory_firmware=$false;framework='ESP-IDF';idf_version='v6.0.2';target='esp32p4';example=$Script:ExpectedExamples[0];variant=$variant.slug;product=$variant.product;resolution=$variant.resolution;panel=$variant.panel;kconfig=$variant.kconfig;revision_profile='rev3_x';revision_bounds=@{min='3.0';max_exclusive=$null};git_sha=('a'*40);baud=460800;offsets=@{'0x1000'='bin/boot.bin';'0x10000'='bin/app.bin'};files=$files;merged_image=@{path='bin/merged-flash.bin';size=1;sha256=(Get-FileSha256 (Join-Path $temp 'bin/merged-flash.bin'))}}; $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $temp 'manifest.json') -Encoding utf8
        $sumPaths = @('bin/app.bin','bin/boot.bin','bin/merged-flash.bin','flash_args','manifest.json'); $sums = $sumPaths | ForEach-Object { "$(Get-FileSha256 (Join-Path $temp $_))  $_" }; Set-Content -LiteralPath (Join-Path $temp 'SHA256SUMS') -Value $sums -Encoding utf8
        $rechecksum = { $sumPaths = @('bin/app.bin','bin/boot.bin','bin/merged-flash.bin','flash_args','manifest.json'); $sums = $sumPaths | ForEach-Object { "$(Get-FileSha256 (Join-Path $temp $_))  $_" }; Set-Content -LiteralPath (Join-Path $temp 'SHA256SUMS') -Value $sums -Encoding utf8 }
        $generated = @(New-Items $definitions $variant.slug ('a'*40)); $item = $generated[0]; $plan = Test-PackageManifest $temp $item ('a'*40); Assert-True ($plan.Count -eq 2) 'SelfTest merged image entered flash plan.'
        $maintainedItem = $generated[-1]; $manifest.artifact_kind = $maintainedItem.ArtifactKind; $manifest.idf_version = $maintainedItem.IdfVersion; $manifest.example = $maintainedItem.Example; $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $temp 'manifest.json') -Encoding utf8; & $rechecksum; $maintainedPlan = Test-PackageManifest $temp $maintainedItem ('a'*40); Assert-True ($maintainedPlan.Count -eq 2) 'SelfTest maintained manifest identity failed.'
        $manifest.artifact_kind = $item.ArtifactKind; $manifest.idf_version = $item.IdfVersion; $manifest.example = $item.Example; $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $temp 'manifest.json') -Encoding utf8; & $rechecksum
        $manifest.schema_version=1; $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $temp 'manifest.json') -Encoding utf8; & $rechecksum; Assert-Rejected { Test-PackageManifest $temp $item ('a'*40) } 'SelfTest schema mismatch was accepted.'; $manifest.schema_version=2; $manifest.flash_size_bytes=16MB; $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $temp 'manifest.json') -Encoding utf8; & $rechecksum; Assert-Rejected { Test-PackageManifest $temp $item ('a'*40) } 'SelfTest flash-size mismatch was accepted.'; $manifest.flash_size_bytes=33554432; $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $temp 'manifest.json') -Encoding utf8; & $rechecksum
        $command = New-EsptoolArguments 'COM12' $plan; Assert-True ((@($command[0..8]) -join '|') -eq '-m|esptool|--port|COM12|--chip|esp32p4|--baud|460800|write_flash') 'SelfTest chip/baud command contract failed.'; Assert-True ((@($command[9..10]) -join '|') -eq '--flash-mode|dio') 'SelfTest write_flash parameter contract failed.'
        Set-Content -LiteralPath (Join-Path $temp 'flash_args') -Value @('--unsafe','value','0x1000','bin/boot.bin') -Encoding utf8; Assert-Rejected { Get-SafeFlashArguments $temp } 'SelfTest unsafe flash parameter was accepted.'; Set-Content -LiteralPath (Join-Path $temp 'flash_args') -Value @('--flash_mode','dio','--flash-mode','qio','--flash_size','32MB','0x1000','bin/boot.bin') -Encoding utf8; Assert-Rejected { Get-SafeFlashArguments $temp } 'SelfTest mixed-alias duplicate flash option was accepted.'; Set-Content -LiteralPath (Join-Path $temp 'flash_args') -Value @('--flash-size','16MB','0x1000','bin/boot.bin') -Encoding utf8; Assert-Rejected { Get-SafeFlashArguments $temp } 'SelfTest non-32MB flash size was accepted.'; Set-Content -LiteralPath (Join-Path $temp 'flash_args') -Value @('--flash-mode','dio','0x1000','bin/boot.bin') -Encoding utf8; Assert-Rejected { Get-SafeFlashArguments $temp } 'SelfTest missing flash size was accepted.'; Set-Content -LiteralPath (Join-Path $temp 'flash_args') -Value @('--flash_mode','dio','--flash_size','32MB','0x1000','bin/boot.bin','0x10000','bin/app.bin') -Encoding utf8; Assert-True ((Get-SafeFlashArguments $temp).Options[0] -eq '--flash_mode') 'SelfTest legacy underscore alias was rejected.'; Set-Content -LiteralPath (Join-Path $temp 'flash_args') -Value @('--flash-mode','dio','--flash-size','32MB','0x1000','bin/boot.bin','0x10000','bin/app.bin') -Encoding utf8; & $rechecksum
        $files[1].offset = '0x1000'; $manifest.files = $files; $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $temp 'manifest.json') -Encoding utf8; & $rechecksum; Assert-Rejected { Test-PackageManifest $temp $item ('a'*40) } 'SelfTest duplicate offset was accepted.'
        $files[1].offset = '0x1002'; $manifest.offsets = @{'0x1000'='bin/boot.bin';'0x1002'='bin/app.bin'}; $manifest.files = $files; Set-Content -LiteralPath (Join-Path $temp 'flash_args') -Value @('--flash_mode','dio','--flash_size','32MB','0x1000','bin/boot.bin','0x1002','bin/app.bin') -Encoding utf8; $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $temp 'manifest.json') -Encoding utf8; & $rechecksum; Assert-Rejected { Test-PackageManifest $temp $item ('a'*40) } 'SelfTest overlapping offset was accepted.'
        $files[1].offset = '0x2000000'; $manifest.offsets = @{'0x1000'='bin/boot.bin';'0x2000000'='bin/app.bin'}; $manifest.files = $files; Set-Content -LiteralPath (Join-Path $temp 'flash_args') -Value @('--flash_mode','dio','--flash_size','32MB','0x1000','bin/boot.bin','0x2000000','bin/app.bin') -Encoding utf8; $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $temp 'manifest.json') -Encoding utf8; & $rechecksum; Assert-Rejected { Test-PackageManifest $temp $item ('a'*40) } 'SelfTest over-32MB offset was accepted.'
        Add-Content -LiteralPath (Join-Path $temp 'bin/app.bin') -Value 'tamper'; try { $null = Test-PackageManifest $temp $item ('a'*40); throw 'SelfTest checksum tamper was accepted.' } catch { if ($_.Exception.Message -eq 'SelfTest checksum tamper was accepted.') { throw } }
        Assert-True ($files[0].offset -eq '0x1000') 'SelfTest setup failed.'
    }
    finally { if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Recurse -Force } }
    Write-Output 'SELF_TEST_OK products=3 examples_per_product=6 progress_reset=sha,product,port package_guards=path,duplicate,checksum,artifact_digest,offsets,merged,chip_baud'
}

if ($SelfTest) { Invoke-SelfTest; return }
$definitions = Get-Definitions
if ($ListOnly) { Write-Output 'inventory=21 (18 example rev3_x artifacts; 3 maintained rev3_x artifacts)'; Write-Output 'finalSHA=resolved-at-runtime; runtime requires clean non-detached branch with upstream, one non-draft open PR at local HEAD, and successful exact-SHA workflow runs.'; foreach ($variant in $definitions.Variants) { foreach ($example in $definitions.Examples) { Write-Output ("example product={0} profile=rev3_x idf=v6.0.2 example={1}" -f $variant.slug,$example) }; Write-Output ("maintained product={0} profile=rev3_x idf=v5.5.5 example=firmware/brookesia" -f $variant.slug) }; return }
if ([string]::IsNullOrWhiteSpace($Product)) { Add-Type -AssemblyName System.Windows.Forms; Add-Type -AssemblyName System.Drawing; $dialog = New-Object System.Windows.Forms.Form; $dialog.Text = 'Select display product'; $dialog.StartPosition = 'CenterScreen'; $dialog.ClientSize = New-Object System.Drawing.Size(430,180); $box = New-Object System.Windows.Forms.ComboBox; $box.DropDownStyle = 'DropDownList'; $box.Location = New-Object System.Drawing.Point(20,25); $box.Size = New-Object System.Drawing.Size(390,25); foreach ($variant in $definitions.Variants) { [void]$box.Items.Add("$($variant.slug) — $($variant.label)") }; $box.SelectedIndex=0; $ok = New-Object System.Windows.Forms.Button; $ok.Text='Continue'; $ok.Location = New-Object System.Drawing.Point(230,90); $cancel=New-Object System.Windows.Forms.Button; $cancel.Text='Cancel'; $cancel.Location=New-Object System.Drawing.Point(330,90); $dialog.Controls.AddRange(@($box,$ok,$cancel)); $ok.Add_Click({ $dialog.DialogResult=[System.Windows.Forms.DialogResult]::OK; $dialog.Close() }); $cancel.Add_Click({ $dialog.DialogResult=[System.Windows.Forms.DialogResult]::Cancel; $dialog.Close() }); if ($dialog.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) { return }; $Product = [string]$definitions.Variants[$box.SelectedIndex].slug }
$git = Resolve-Git; $finalSha = Resolve-FinalSha $git; $branch = Assert-LocalGitReady $git; $gh = Resolve-Gh; Assert-ReadyPullRequest $gh $branch $finalSha; $python = Resolve-PythonWithEsptool; $explicitPort = $Port; $defaultPort = if ([string]::IsNullOrWhiteSpace($explicitPort)) { Resolve-DefaultPort } else { '' }; $initialPort = Resolve-SelectedPort $explicitPort '' $defaultPort; $initialSilicon = Get-SiliconRevision $python $initialPort; $profile = Get-RevisionProfile $initialSilicon; $items = New-Items $definitions $Product $finalSha $profile; $runId = Resolve-ArtifactRun $gh $finalSha $items
$state = Read-State $Product $finalSha $items.Count $profile $initialSilicon.Dotted $initialSilicon.DeviceId; $Port = Resolve-SelectedPort $explicitPort $state.Port $defaultPort; $silicon = $initialSilicon; if ($Port -ne $initialPort) { $silicon = Get-SiliconRevision $python $Port; Assert-SameSiliconIdentity $initialSilicon $silicon; Assert-RevisionProfile $profile $silicon }
Add-Type -AssemblyName System.Windows.Forms; Add-Type -AssemblyName System.Drawing
$script:CurrentIndex=$state.CurrentIndex; $script:Confirmed=@($state.Confirmed); $script:FlashVerified=$false
$form=New-Object System.Windows.Forms.Form; $form.Text='CI Firmware Flasher'; $form.StartPosition='CenterScreen'; $form.ClientSize=New-Object System.Drawing.Size(900,690); $form.FormBorderStyle='FixedDialog'; $form.MaximizeBox=$false
function Add-Label([string]$Text,[int]$X,[int]$Y,[int]$Width=860) { $label=New-Object System.Windows.Forms.Label; $label.Text=$Text; $label.Location=New-Object System.Drawing.Point($X,$Y); $label.Size=New-Object System.Drawing.Size($Width,20); $form.Controls.Add($label); return $label }
$null=Add-Label "Product: $Product ($($items[0].Variant.label))" 15 15; $null=Add-Label "Final SHA: $finalSha | example IDF: $Script:ExampleIdfVersion | maintained IDF: $Script:MaintainedIdfVersion | silicon: $($silicon.Dotted) | MAC: $($silicon.DeviceId) | profile: $profile" 15 40; $null=Add-Label 'Warning: silicon revision and MAC identity do not replace PCB/electrical revision confirmation.' 15 60; $null=Add-Label 'Port:' 15 85 45; $portBox=New-Object System.Windows.Forms.TextBox; $portBox.Text=$Port; $portBox.Location=New-Object System.Drawing.Point(65,82); $portBox.Size=New-Object System.Drawing.Size(110,22); $form.Controls.Add($portBox); $currentLabel=Add-Label '' 15 115; $statusLabel=Add-Label 'Status: Select Flash current to begin.' 15 140
$progress=New-Object System.Windows.Forms.ListBox; $progress.Font=New-Object System.Drawing.Font('Consolas',9); $progress.Location=New-Object System.Drawing.Point(15,155); $progress.Size=New-Object System.Drawing.Size(870,180); $form.Controls.Add($progress); $output=New-Object System.Windows.Forms.TextBox; $output.Multiline=$true; $output.ReadOnly=$true; $output.ScrollBars='Both'; $output.WordWrap=$false; $output.Font=New-Object System.Drawing.Font('Consolas',9); $output.Location=New-Object System.Drawing.Point(15,350); $output.Size=New-Object System.Drawing.Size(870,250); $form.Controls.Add($output)
$flash=New-Object System.Windows.Forms.Button; $flash.Text='Flash current'; $flash.Location=New-Object System.Drawing.Point(15,620); $flash.Size=New-Object System.Drawing.Size(145,32); $form.Controls.Add($flash); $confirm=New-Object System.Windows.Forms.Button; $confirm.Text='Mark PASS and flash next'; $confirm.Location=New-Object System.Drawing.Point(170,620); $confirm.Size=New-Object System.Drawing.Size(215,32); $confirm.Enabled=$false; $form.Controls.Add($confirm); $exit=New-Object System.Windows.Forms.Button; $exit.Text='Exit'; $exit.Location=New-Object System.Drawing.Point(765,620); $exit.Size=New-Object System.Drawing.Size(120,32); $form.Controls.Add($exit)
function Update-Display { $item=$items[$script:CurrentIndex-1]; $currentLabel.Text="Current: $($item.Index)/$($items.Count) $($item.Example)"; $progress.Items.Clear(); foreach ($entry in $items) { $mark=if($script:Confirmed -contains $entry.Index){'[PASS]'}elseif($entry.Index -eq $script:CurrentIndex){'[CURRENT]'}else{'[WAIT]'}; [void]$progress.Items.Add("$mark $($entry.Index): $($entry.Example)") }; $progress.SelectedIndex=$script:CurrentIndex-1 }
function Set-Busy([bool]$Busy) { $done=$script:CurrentIndex -eq $items.Count -and $script:Confirmed -contains $items.Count; $flash.Enabled=(-not $Busy)-and(-not $done); $confirm.Enabled=(-not $Busy)-and $script:FlashVerified-and(-not $done); $portBox.Enabled=-not $Busy; $exit.Enabled=-not $Busy; $form.UseWaitCursor=$Busy; [System.Windows.Forms.Application]::DoEvents() }
function Flash-Current { $selected=$portBox.Text.Trim().ToUpperInvariant(); if(-not(Test-Port $selected)){[System.Windows.Forms.MessageBox]::Show('Port must be COM followed by digits.','Invalid port')|Out-Null;return}; $script:FlashVerified=$false; Set-Busy $true; $item=$items[$script:CurrentIndex-1]; $statusLabel.Text="Status: Probing $selected before flashing $($item.Example)..."; try { $currentSilicon=Get-SiliconRevision $python $selected; Assert-SameSiliconIdentity $silicon $currentSilicon; Assert-RevisionProfile $item.Profile $currentSilicon; $statusLabel.Text="Status: Flashing $($item.Example) on $selected..."; $result=Invoke-CurrentFlash $item $selected $item.RunId $gh $python $finalSha; $output.Text="Log: $($result.LogPath)`r`n`r`n$($result.Output)"; if($result.Success){$script:FlashVerified=$true;$statusLabel.Text='Status: Flash verified. Check the device, then mark PASS to persist and advance.'}else{$statusLabel.Text="Status: Flash verification failed; current item was not advanced. Log: $($result.LogPath)"} } catch { $output.Text=$_|Out-String; $statusLabel.Text="Status: Error; current item was not advanced. $($_.Exception.Message)" } finally { Set-Busy $false } }
$flash.Add_Click({Flash-Current}); $confirm.Add_Click({if(-not $script:FlashVerified){return};$selected=$portBox.Text.Trim().ToUpperInvariant();$next=Get-NextProgress $script:CurrentIndex $script:Confirmed $items.Count;$script:CurrentIndex=$next.CurrentIndex;$script:Confirmed=@($next.Confirmed);$script:FlashVerified=$false;Save-State $script:CurrentIndex $script:Confirmed $selected $Product $finalSha $profile $silicon.Dotted $silicon.DeviceId;Update-Display;if($next.Completed){$statusLabel.Text='Status: All examples are confirmed.';Set-Busy $false;return};Flash-Current}); $exit.Add_Click({$form.Close()}); $progress.Add_SelectedIndexChanged({if($progress.SelectedIndex -ne ($script:CurrentIndex-1)){$progress.SelectedIndex=$script:CurrentIndex-1}}); Update-Display; Set-Busy $false; [void]$form.ShowDialog()
