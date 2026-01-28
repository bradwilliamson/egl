param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot ".."))
)

$ErrorActionPreference = 'Stop'

function Remove-CComments {
    param([string]$Text)

    $NORMAL = 0
    $LINE_COMMENT = 1
    $BLOCK_COMMENT = 2
    $STRING = 3
    $CHAR = 4

    $state = $NORMAL
    $escaped = $false

    $sb = New-Object System.Text.StringBuilder

    for ($i = 0; $i -lt $Text.Length; $i++) {
        $c = $Text[$i]
        $next = if ($i + 1 -lt $Text.Length) { $Text[$i + 1] } else { [char]0 }

        switch ($state) {
            $NORMAL {
                if ($c -eq '/' -and $next -eq '/') {
                    # start line comment
                    [void]$sb.Append(' ')
                    [void]$sb.Append(' ')
                    $state = $LINE_COMMENT
                    $i++
                    continue
                }
                if ($c -eq '/' -and $next -eq '*') {
                    # start block comment
                    [void]$sb.Append(' ')
                    [void]$sb.Append(' ')
                    $state = $BLOCK_COMMENT
                    $i++
                    continue
                }
                if ($c -eq '"') {
                    [void]$sb.Append($c)
                    $state = $STRING
                    $escaped = $false
                    continue
                }
                if ($c -eq "'") {
                    [void]$sb.Append($c)
                    $state = $CHAR
                    $escaped = $false
                    continue
                }

                [void]$sb.Append($c)
            }

            $LINE_COMMENT {
                if ($c -eq "`n") {
                    [void]$sb.Append($c)
                    $state = $NORMAL
                } else {
                    # preserve column count
                    [void]$sb.Append(' ')
                }
            }

            $BLOCK_COMMENT {
                if ($c -eq '*' -and $next -eq '/') {
                    [void]$sb.Append(' ')
                    [void]$sb.Append(' ')
                    $state = $NORMAL
                    $i++
                } elseif ($c -eq "`n") {
                    [void]$sb.Append($c)
                } else {
                    [void]$sb.Append(' ')
                }
            }

            $STRING {
                [void]$sb.Append($c)
                if ($escaped) {
                    $escaped = $false
                } elseif ($c -eq '\\') {
                    $escaped = $true
                } elseif ($c -eq '"') {
                    $state = $NORMAL
                }
            }

            $CHAR {
                [void]$sb.Append($c)
                if ($escaped) {
                    $escaped = $false
                } elseif ($c -eq '\\') {
                    $escaped = $true
                } elseif ($c -eq "'") {
                    $state = $NORMAL
                }
            }
        }
    }

    return $sb.ToString()
}

$targets = @(
    (Join-Path (Join-Path $RepoRoot "game") "*.c"),
    (Join-Path (Join-Path $RepoRoot "xatrix") "*.c"),
    (Join-Path (Join-Path $RepoRoot "rogue") "*.c")
)

$pattern = [regex]'\bAddCommandString\s*\(\s*"joy_rumble'

$failures = @()

foreach ($glob in $targets) {
    foreach ($file in (Get-ChildItem -Path $glob -ErrorAction SilentlyContinue)) {
        $raw = Get-Content -LiteralPath $file.FullName -Raw
        $stripped = Remove-CComments -Text $raw

        if ($pattern.IsMatch($stripped)) {
            # Report the first match line number for fast fixing
            $lines = $stripped -split "\r?\n"
            for ($ln = 0; $ln -lt $lines.Length; $ln++) {
                if ($pattern.IsMatch($lines[$ln])) {
                    $failures += [pscustomobject]@{
                        File = $file.FullName
                        Line = $ln + 1
                        Snippet = ($lines[$ln].Trim())
                    }
                    break
                }
            }
        }
    }
}

if ($failures.Count -gt 0) {
    Write-Host "ERROR: Found server-side joy_rumble command injection (AddCommandString(\"joy_rumble...\"))" -ForegroundColor Red
    $failures | Format-Table -AutoSize
    exit 1
}

Write-Host "OK: No server-side joy_rumble AddCommandString injection found." -ForegroundColor Green
exit 0
