$ErrorActionPreference = "Stop"

$oggurl = "https://downloads.xiph.org/releases/ogg/libogg-1.3.4.zip"
$oggfilesum = "DD74E3AE52BEAB6C894D4B721DB786961E64F073F28EF823C5D2A3558D4FAB2D"
$oggmsbuildopts = @(
    "win32\VS2015\libogg.sln"
)

$vorbisurl = "https://downloads.xiph.org/releases/vorbis/libvorbis-1.3.7.zip"
$vorbisfilesum = "57C8BC92D2741934B8DC939AF49C2639EDC44B8879CBA2EC14AD3189E2814582"
$vorbismsbuildopts = @(
    "win32\VS2010\vorbis_static.sln",
     "/p:LIBOGG_VERSION=1.3.4",
     "/p:PlatformToolset=v140",
     "/p:WindowsTargetPlatformVersion=8.1"
)

$destdir = (Get-Item .).FullName

function check_tools() {
    Write-Host "+++ Checking build tools"
    & "nmake"
}

function check_file($shasum, $filename) {
    $hash = Get-FileHash -Algorithm SHA256 -Path $filename
    return $hash.Hash -eq $shasum
}

function prepare_package($url, $shasum) {
    $url = [System.Uri]::new($url)
    $filename = $url.Segments[-1]
    $dirname = $filename.Substring(0, $filename.LastIndexOf("."))

    $download = $False
    $oncealready = $False
    do {
        $file = $Null
        if ($download) {
            Write-Host "+++ Fetching $url"
            Invoke-WebRequest -Uri $url -OutFile $filename
        }
        try {
            $file = Get-Item $filename
            if (!(check_file $shasum $file.FullName)) {
                Write-Warning "$filename checksum is incorrect. Redownloading."
                $file.Delete()
                $file = $Null
                $download = $True
            }
        } catch [System.Management.Automation.ItemNotFoundException] {
            $download = $True
        }
        if (!$file -And $oncealready) {
            Write-Error "Could not download $url successfully."
        }
        $oncealready = $True
    } while (!$file)

    try {
        $dir = Get-Item $dirname
    } catch [System.Management.Automation.ItemNotFoundException] {
        Write-Host "+++ Unpacking $filename"

        Expand-Archive $filename -DestinationPath . -Force
        $dir = Get-Item $dirname
    }

    return $dir
}

function ensure_directory($dirname) {
    try {
        $dir = Get-Item $dirname
    } catch [System.Management.Automation.ItemNotFoundException] {
        $dir = New-Item $dirname -Type directory
    }
    return $dir
}

$includedir = ensure_directory "$destdir\include"
$libx86dir = ensure_directory "$destdir\lib"
$libx64dir = ensure_directory "$destdir\libx64"

if ($True) {
    Write-Host "+++ Preparing libogg"
    $oggdir = prepare_package $oggurl $oggfilesum
    $oggincdir = ensure_directory "$destdir\include\ogg"

    Write-Host "+++ Building libogg"
    Push-Location $oggdir
    & msbuild @oggmsbuildopts "/p:Configuration=Release;Platform=Win32"
    & msbuild @oggmsbuildopts "/p:Configuration=Release;Platform=x64"
    Pop-Location

    Write-Host "+++ Installing libogg"
    Copy-Item "$oggdir\include\ogg\*.h" $oggincdir -Force
    Copy-Item "$oggdir\win32\*\Win32\Release\libogg.lib" $libx86dir -Force
    Copy-Item "$oggdir\win32\*\x64\Release\libogg.lib" $libx64dir -Force
}

if ($True) {
    Write-Host "+++ Preparing libvorbis"
    $vorbisdir = prepare_package $vorbisurl $vorbisfilesum
    $vorbisincdir = ensure_directory "$destdir\include\vorbis"

    Write-Host "+++ Building libvorbis"
    Push-Location $vorbisdir
    & msbuild @vorbismsbuildopts "/p:Configuration=Release;Platform=Win32"
    & msbuild @vorbismsbuildopts "/p:Configuration=Release;Platform=x64"
    Pop-Location

    Write-Host "+++ Installing libvorbis"
    Copy-Item "$vorbisdir\include\vorbis\*.h" $vorbisincdir -Force
    Copy-Item "$vorbisdir\win32\*\Win32\Release\libvorbis_static.lib" $libx86dir -Force
    Copy-Item "$vorbisdir\win32\*\Win32\Release\libvorbisfile_static.lib" $libx86dir -Force
    Copy-Item "$vorbisdir\win32\*\x64\Release\libvorbis_static.lib" $libx64dir -Force
    Copy-Item "$vorbisdir\win32\*\x64\Release\libvorbisfile_static.lib" $libx64dir -Force
}
