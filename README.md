# ReLCS-Cleo

[![Build ReLCS-Cleo.asi](https://github.com/syahdafahreza/ReLCS-PC-Ryadica926/actions/workflows/build.yml/badge.svg)](https://github.com/syahdafahreza/ReLCS-PC-Ryadica926/actions/workflows/build.yml)

Plugin `.asi` modern, ringan, dan bebas crash khusus untuk **GTA Liberty City Stories PC (Re:LCS by Ryadica926)**. Dilengkapi dengan **In-Game Cheat Menu (Direct3D 8 Overlay)**, **35 Cheat Bawaan LCS Lengkap**, dan **Crash Logger Interceptor**.

---

## 💡 Mengapa ReLCS Butuh Plugin Khusus?

Saat menginstal plugin CLEO standar Vice City (`VC.CLEO.asi`, CLEO 4, maupun CLEO Redux), game langsung mengalami:
```text
Unhandled exception: c0000005 (Access Violation Error)
```
**Penyebabnya:**  
Game Re:LCS menggunakan `gta-lcs.exe` modifikasi buatan Ryadica926 yang tata letak memori RAM-nya (memory space) sudah dirombak total untuk memuat map Liberty City, model, animasi, dan script misi LCS. Ketika plugin CLEO Vice City standar mencoba mengakses alamat memori hardcoded Vice City original, terjadi tabrakan alokasi memori seketika.

**Solusi ReLCS-Cleo:**  
`ReLCS-Cleo.asi` dirancang mandiri (standalone) tanpa ketergantungan pada pustaka CLEO yang bentrok. Plugin ini dimuat langsung melalui **Ultimate ASI Loader** (`d3d8.dll` bawaan game) dan memanfaatkan Direct3D 8 Presentation Hook untuk merender overlay visual serta aktivasi cheat yang 100% aman dan stabil.

---

## ✨ Fitur Utama

1. **In-Game D3D8 Overlay Menu (`F9`)**
   - Tampilan menu modern langsung di atas rendering game tanpa menurunkan performa.
   - Menggunakan embedded bitmap font 8x8 yang dirender langsung via Direct3D 8 (tidak memerlukan D3DX atau font eksternal).
   - Animasi notifikasi cheat ("CHEAT ACTIVATED") yang halus.

2. **Semua 35 Cheat Bawaan LCS Lengkap:**
   - **Player Cheats (1-10):**
     - Full Health
     - Full Armor
     - $250,000 + Health + Armor
     - Weapon Set 1 (Brass knuckles, knife, Molotov, 9mm, shotgun, tec-9, AK-47, sniper, flamethrower)
     - Weapon Set 2 (Katana, grenade, .357, stubby shotgun, Uzi, M4, rocket launcher, laser sniper)
     - Weapon Set 3 (Chainsaw, grenade, .357, spaz shotgun, MP5, M4, minigun, laser sniper)
     - Clear Wanted Level
     - Raise Wanted Level (+2 Bintang)
     - Never Wanted (Bintang terkunci di 0)
     - Bunuh Diri (Commit Suicide)
   - **Vehicle Cheats (11-19):**
     - Spawn Rhino Tank
     - Spawn Trashmaster (Truk Sampah)
     - Spawn Deaddodo (Pesawat Dodo)
     - Ledakkan Semua Mobil Sekitar (Blow Up Cars)
     - Perfect Traction & Handling
     - Pengemudi Agresif (Aggressive Drivers)
     - Mobil Putih / Chrome
     - Mobil Hitam
     - Mobil Bisa Berjalan di Atas Air (Cars Drive On Water)
   - **World Cheats (20-31):**
     - Cuaca Cerah (Sunny)
     - Cuaca Sangat Bersih (Clear)
     - Cuaca Mendung (Overcast)
     - Cuaca Hujan Deras (Rainy)
     - Cuaca Berkabut (Foggy)
     - Percepat Waktu Jam Game (Faster Clock)
     - Semua Lampu Lalu Lintas Hijau
     - Semua Lampu Lalu Lintas Merah
     - Pejalan Kaki Tawuran (Pedestrian Riot)
     - Pejalan Kaki Menyerang Pemain (Ped Attack You)
     - Pejalan Kaki Bersenjata (Armed Pedestrians)
     - Pejalan Kaki Mengikuti Pemain (Ped Follow You)
   - **Visual & Gameplay Cheats (32-35):**
     - Slow Motion (Gerak Lambat)
     - Fast Motion (Gerak Cepat)
     - Layar Terbalik (Upside Down Screen)
     - Orientasi Layar Normal (Normal Screen Orientation)

3. **Real-Time FPS Counter (`F10`)**
   - Menampilkan angka FPS permainan secara akurat di pojok layar.

4. **Crash Logger Interceptor:**
   - Menangkap error `c0000005` sebelum game force close tanpa jejak.
   - Mencatat nilai register x86 CPU (`EAX`, `EBX`, `ECX`, `EDX`, `ESP`, `EBP`, `EIP`), modul yang menyebabkan crash, serta stack trace ke `scripts/crash.log`.
   - Membuat file minidump `scripts/crash.dmp` untuk dianalisis melalui Visual Studio / WinDbg.

---

## 🎮 Kontrol / Tombol

| Tombol | Fungsi |
| :--- | :--- |
| **`F9`** | Buka / Tutup Menu Cheat |
| **`F10`** | Toggle FPS Counter |
| **`Panah Atas / Bawah`** | Navigasi & Memilih Cheat |
| **`Panah Kiri / Kanan`** | Ganti Kategori (Player / Vehicle / World / Visual) |
| **`Enter`** | Aktifkan Cheat Terpilih |
| **`Esc` / `Backspace`** | Tutup Menu |

---

## 📥 Panduan Instalasi

1. Download file `ReLCS-Cleo.asi` terbaru dari tab [Releases](../../releases) atau dari artifact GitHub Actions.
2. Pastikan file `d3d8.dll` (Ultimate ASI Loader) sudah berada di folder instalasi GTA Liberty City Stories Anda.
3. Salin `ReLCS-Cleo.asi` ke dalam subfolder `scripts\`:
   ```text
   GTA Liberty City Stories/
   ├── gta-lcs.exe
   ├── d3d8.dll
   └── scripts/
       └── ReLCS-Cleo.asi
   ```
4. Jalankan game dan tekan tombol **`F9`** saat di dalam game untuk membuka menu!

---

## 🛠️ Kompilasi dari Source Code

Proyek ini menggunakan **Visual Studio 2022** (Platform Toolset `v143`, C++20, Win32 / x86):

```pwsh
msbuild ReLCS-Cleo.sln /p:Configuration=Release /p:Platform=Win32 /p:OutDir="build\\"
```

File output `ReLCS-Cleo.asi` akan dibuat secara otomatis di dalam direktori `build/`.

Atau Anda cukup push ke branch `main`, dan **GitHub Actions** akan secara otomatis mengompilasi dan mengunggah artifact `ReLCS-Cleo.asi`.

---

## 📄 Lisensi & Kredit

- **Ryadica926**: Pembuat port dan total conversion GTA Re:LCS PC.
- Dikembangkan khusus untuk komunitas GTA Liberty City Stories PC.
