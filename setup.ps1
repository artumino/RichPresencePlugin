# Retrieve submodules
Write-Host "Updating git submodules..."
git submodule update --init --recursive

# --------- Build
Write-Host "------------------------------------"
Write-Host "Attempting to build RichPresence"
Write-Host "------------------------------------"

# Find and bind msbuild - thank you Lex Li
Write-Host "Searching for msbuild.exe..."
$msBuild = "msbuild"
try
{
    & $msBuild /version
}
catch
{
    Write-Host "MSBuild doesn't exist. Using VSSetup instead."
    Install-Module VSSetup -Scope CurrentUser -Force
    $instance = Get-VSSetupInstance -All -Prerelease | Select-VSSetupInstance -Require 'Microsoft.Component.MSBuild' -Latest
    $installDir = $instance.installationPath
    Write-Host "Visual Studio is found at $installDir"
    $msBuild = $installDir + '\MSBuild\Current\Bin\MSBuild.exe' # VS2019
    if (![System.IO.File]::Exists($msBuild))
    {
        $msBuild = $installDir + '\MSBuild\15.0\Bin\MSBuild.exe' # VS2017
        if (![System.IO.File]::Exists($msBuild))
        {
            Write-Host "MSBuild doesn't exist. Exit."
            exit 1
        }
    }
}
Write-Host "MSBuild found"

# Build Revive a la carte
Write-Host "Building OpenSteamworks..."
& $msBuild 'Externals\open-steamworks\Open Steamworks.sln' /t:Build /p:Configuration=Release /p:Platform=x64
& $msBuild 'Externals\open-steamworks\Open Steamworks.sln' /t:Build /p:Configuration=Release /p:Platform=Win32


Write-Host "Building RichPresence Plugin..."
& $msBuild 'RichPresencePlugin.sln' /t:Build /p:Configuration=Release /p:Platform=x64
& $msBuild 'RichPresencePlugin.sln' /t:Build /p:Configuration=Release /p:Platform=Win32
