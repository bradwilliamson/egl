param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot ".."))
)

$ErrorActionPreference = 'Stop'

function Assert-Contains {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Needle,
        [Parameter(Mandatory = $true)][string]$Message
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Missing file: $Path"
    }

    $text = Get-Content -LiteralPath $Path -Raw
    if ($text -notmatch [regex]::Escape($Needle)) {
        throw $Message
    }
}

$mp = Join-Path $RepoRoot "cgame\menu\m_mp.c"
$menu = Join-Path $RepoRoot "cgame\menu\menu.c"
$dl = Join-Path $RepoRoot "cgame\menu\m_mp_downloading.c"

# Ensure the menu entry exists + is wired
Assert-Contains -Path $mp -Needle "dlopts_menu" -Message "Multiplayer menu is missing dlopts_menu (download options entry)."
Assert-Contains -Path $mp -Needle "Download Options" -Message "Multiplayer menu is missing the 'Download Options' label."
Assert-Contains -Path $mp -Needle "DLOpts_Menu" -Message "Multiplayer menu is not wired to DLOpts_Menu callback."
Assert-Contains -Path $mp -Needle "UI_DLOptionsMenu_f" -Message "Multiplayer download options callback should invoke UI_DLOptionsMenu_f (missing reference)."

# Ensure the command still exists (compat / console access)
Assert-Contains -Path $menu -Needle "menu_dloptions" -Message "menu_dloptions command is missing from cgame/menu/menu.c."
Assert-Contains -Path $menu -Needle "UI_DLOptionsMenu_f" -Message "UI_DLOptionsMenu_f is not registered with menu_dloptions command."

# Ensure the actual menu implementation exists
Assert-Contains -Path $dl -Needle "UI_DLOptionsMenu_f" -Message "Download options menu implementation (UI_DLOptionsMenu_f) is missing."
Assert-Contains -Path $dl -Needle "allow_download" -Message "Download options menu no longer references allow_download cvars."

Write-Host "OK: Download Options menu is present and wired." -ForegroundColor Green
exit 0
