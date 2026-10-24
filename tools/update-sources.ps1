<#
.SYNOPSIS
    Atualiza a lista de arquivos do qt_add_executable no CMakeLists.txt
    com os arquivos de código que existem na pasta src/.

.DESCRIPTION
    A lista é sempre recriada a partir do que está no disco, então o script
    cobre arquivos adicionados, excluídos e renomeados (inclusive pastas).
    O CMakeLists.txt só é gravado quando a lista realmente muda.

.EXAMPLE
    .\tools\update-sources.ps1
    Atualiza a lista uma vez.

.EXAMPLE
    .\tools\update-sources.ps1 -Watch
    Fica observando a pasta src/ e atualiza a cada arquivo criado, excluído
    ou renomeado. Ctrl+C para parar.

.EXAMPLE
    .\tools\update-sources.ps1 -Check
    Só verifica: mostra o que mudaria e termina com código 1 se a lista
    estiver desatualizada, sem alterar nada.
#>
param(
    [switch]$Watch,
    [switch]$Check
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$cmakeFile = Join-Path $root 'CMakeLists.txt'
$srcDir = Join-Path $root 'src'
$extensions = @('.h', '.hpp', '.cpp', '.ui', '.qrc')
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

# Cabeçalhos antes do .cpp de mesmo nome, como no resto do CMakeLists.txt
function Get-ExtensionRank([string]$extension) {
    switch ($extension.ToLowerInvariant()) {
        { $_ -in '.h', '.hpp' } { 0 }
        '.cpp' { 1 }
        default { 2 }
    }
}

function Get-SourceFiles {
    Get-ChildItem $srcDir -Recurse -File |
        Where-Object { $extensions -contains $_.Extension.ToLowerInvariant() } |
        Sort-Object @{ Expression = { $_.FullName.Substring(0, $_.FullName.Length - $_.Extension.Length) } },
                    @{ Expression = { Get-ExtensionRank $_.Extension } } |
        ForEach-Object { $_.FullName.Substring($root.Length + 1) -replace '\\', '/' }
}

# Retorna $true se a lista estava desatualizada
function Update-CMakeLists {
    $content = [IO.File]::ReadAllText($cmakeFile, $utf8NoBom)

    $match = [regex]::Match($content, '(?s)(qt_add_executable\(\s*\$\{PROJECT_NAME\})(.*?)(\r?\n\))')
    if (-not $match.Success) {
        throw "Não encontrei o bloco 'qt_add_executable(`${PROJECT_NAME} ... )' no CMakeLists.txt."
    }

    $newline = if ($content.Contains("`r`n")) { "`r`n" } else { "`n" }
    $files = @(Get-SourceFiles)
    $oldFiles = @($match.Groups[2].Value -split '\s+' | Where-Object { $_ })

    $list = ($files | ForEach-Object { "$newline    $_" }) -join ''
    $updated = $content.Substring(0, $match.Index) +
               $match.Groups[1].Value + $list + $newline + ')' +
               $content.Substring($match.Index + $match.Length)

    if ($updated -eq $content) {
        Write-Host "CMakeLists.txt já está atualizado ($($files.Count) arquivos)."
        return $false
    }

    $added = $files | Where-Object { $_ -notin $oldFiles }
    $removed = $oldFiles | Where-Object { $_ -notin $files }
    $added | ForEach-Object { Write-Host "  + $_" -ForegroundColor Green }
    $removed | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
    if (-not $added -and -not $removed) { Write-Host '  (só a ordem dos arquivos mudou)' }

    if ($Check) {
        Write-Host 'CMakeLists.txt está desatualizado.' -ForegroundColor Yellow
        return $true
    }

    [IO.File]::WriteAllText($cmakeFile, $updated, $utf8NoBom)
    Write-Host "CMakeLists.txt atualizado ($($files.Count) arquivos)." -ForegroundColor Cyan
    return $true
}

if (-not $Watch) {
    $outdated = Update-CMakeLists
    if ($Check -and $outdated) { exit 1 }
    exit 0
}

Update-CMakeLists | Out-Null

$watcher = New-Object IO.FileSystemWatcher $srcDir
$watcher.IncludeSubdirectories = $true
$watcher.NotifyFilter = [IO.NotifyFilters]'FileName, DirectoryName'
$changeTypes = [IO.WatcherChangeTypes]'Created, Deleted, Renamed'

Write-Host "Observando $srcDir. Ctrl+C para parar."
try {
    while ($true) {
        $change = $watcher.WaitForChanged($changeTypes, 1000)
        if ($change.TimedOut) { continue }

        # O Visual Studio costuma gerar várias mudanças seguidas (arquivos
        # temporários ao salvar, por exemplo); espera um pouco para juntá-las.
        Start-Sleep -Milliseconds 300
        Update-CMakeLists | Out-Null
    }
}
finally {
    $watcher.Dispose()
}
