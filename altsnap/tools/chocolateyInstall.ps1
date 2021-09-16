$ErrorActionPreference = 'Stop';

$packageName = 'altdrag'
$toolsDir   = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
$url        = 'https://github.com/RamonUnch/AltSnap/releases/download/1.47/AltSnap1.47-inst.exe'
$url64      = 'https://github.com/RamonUnch/AltSnap/releases/download/1.47/AltSnap1.47-x64-inst.exe'

$packageArgs = @{
  packageName   = $packageName
  unzipLocation = $toolsDir
  fileType      = 'exe'

  url           = $url
  url64bit      = $url64
  checksum      = '46C8338616A2A9EA07245616A90D511639F9A8222091491CAF9D209BC6FD3346'
  checksumType  = 'sha256'
  checksum64    = '46C8338616A2A9EA07245616A90D511639F9A8222091491CAF9D209BC6FD3346'
  checksumType64= 'sha256'

  silentArgs   = '/S'

  softwareName  = 'AltSnap'
}

Install-ChocolateyPackage @packageArgs
