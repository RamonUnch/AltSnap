# GitHub Actions Build System

Bu proje için Windows x64 ve x86 derlemeleri yapmak üzere çeşitli GitHub Actions workflow'ları oluşturulmuştur.

## Workflow Dosyaları

### 1. `build-windows-x64.yml` - Ana x64 Derleme

- **Tetikleme**: `main` ve `develop` branch'lerine push ve PR'lar
- **Çıktı**: Windows x64 için AltSnap.exe ve hooks.dll
- **Manuel tetikleme**: Workflow dispatch ile mümkün

### 2. `build-windows-x86.yml` - x86 Derleme

- **Tetikleme**: `main` ve `develop` branch'lerine push ve PR'lar
- **Çıktı**: Windows x86 için AltSnap.exe ve hooks.dll
- **Manuel tetikleme**: Workflow dispatch ile mümkün

### 3. `build-matrix.yml` - Çoklu Platform Derleme

- **Tetikleme**: `main` branch'e push ve PR'lar
- **Çıktı**: Hem x64 hem x86 sürümlerini tek seferde derler
- **Özel özellik**: Tüm sürümleri birleştiren combined artifact oluşturur

### 4. `release.yml` - Otomatik Release

- **Tetikleme**: `v*` formatında tag'ler (örn: v1.0.0, v2.1.0)
- **Çıktı**: GitHub Release sayfasında zip dosyası ve executable'lar
- **İçerik**: Tam kurulum paketi (lang dosyaları, themes, dokümantasyon)

### 5. `test-build.yml` - Test Derlemesi

- **Tetikleme**: `develop`, `feature/*`, `bugfix/*` branch'lerine push
- **Amaç**: Hızlı derleme testi, hata kontrolü
- **Çıktı**: Sadece derleme başarısı/başarısızlığı

## Kullanım

### Normal Geliştirme

1. `develop` branch'inde çalışın
2. Her commit'te test build çalışır
3. `main`'e merge ettiğinizde full build çalışır

### Release Oluşturma

1. Kod hazır olduğunda tag oluşturun:

```bash
git tag v1.0.0
git push origin v1.0.0
```

1. Otomatik olarak release oluşturulur

### Manuel Build

1. GitHub'da Actions sekmesine gidin
2. İstediğiniz workflow'u seçin
3. "Run workflow" butonuna tıklayın

## Artifacts

Build'ler tamamlandığında aşağıdaki dosyalar indirilabilir:

- `AltSnap.exe` - Ana uygulama
- `hooks.dll` - Sistem hook'ları
- `README.md` - Dokümantasyon
- `License.txt` - Lisans bilgisi
- `Lang/` - Dil dosyaları (release'lerde)
- `Themes/` - Tema dosyaları (release'lerde)

## Gereksinimler

Workflow'lar şunları kullanır:

- **MSYS2/MINGW64** - x64 derlemeler için
- **MSYS2/MINGW32** - x86 derlemeler için  
- **GCC compiler** - C kodu derlemek için
- **windres** - Windows resource dosyaları için

## Troubleshooting

### Build Başarısız Olursa

1. Build log'larını kontrol edin
2. Compiler hataları için kod'u gözden geçirin
3. Makefile değişiklikleri gerekebilir

### Release Oluşturulamazsa

1. Tag formatını kontrol edin (`v` ile başlamalı)
2. GITHUB_TOKEN izinlerini kontrol edin
3. Repository settings/Actions izinlerini kontrol edin

## Konfigürasyon

Workflow'ları özelleştirmek için `.github/workflows/` altındaki YAML dosyalarını düzenleyin.

Önemli parametreler:

- `retention-days`: Artifact'ların ne kadar süre saklanacağı
- `branches`: Hangi branch'lerde çalışacağı
- `install`: MSYS2'de kurulacak paketler
